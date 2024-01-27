
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <libgen.h> /* dirname() */

#include "main.h"
#include "conf.h"
#include "log.h"
#include "unix.h"


static void shutdown_handler(int signo)
{
    // exit on second stop request
    if (!gconf->is_running) {
        exit(1);
    }

    gconf->is_running = false;
}

void unix_signals(void)
{
    struct sigaction sig_stop;
    struct sigaction sig_term;

    // STRG+C aka SIGINT => Stop the program
    sig_stop.sa_handler = shutdown_handler;
    sig_stop.sa_flags = 0;
    if ((sigemptyset(&sig_stop.sa_mask) == -1) || (sigaction(SIGINT, &sig_stop, NULL) != 0)) {
        log_error("Failed to set SIGINT handler: %s", strerror(errno));
        exit(1);
    }

    // SIGTERM => Stop the program gracefully
    sig_term.sa_handler = shutdown_handler;
    sig_term.sa_flags = 0;
    if ((sigemptyset(&sig_term.sa_mask) == -1) || (sigaction(SIGTERM, &sig_term, NULL) != 0)) {
        log_error("Failed to set SIGTERM handler: %s", strerror(errno));
        exit(1);
    }

    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
}

bool unix_create_unix_socket(const char path[], int *sock_out)
{
    struct sockaddr_un addr = {0};
    int sock = -1;

    if (path == NULL || strlen(path) == 0) {
        goto err;
    }

    if (remove(path) == -1 && errno != ENOENT) {
        log_warning("remove() %s", strerror(errno));
    }

    sock = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock < 0) {
        log_error("socket(): %s", strerror(errno));
        goto err;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
        log_error("bind() %s %s", path, strerror(errno));
        goto err;
    }

    if (listen(sock, 5) == -1) {
        goto err;
    }

    *sock_out = sock;

    return true;

err:

    if (sock >= 0) {
        unix_remove_unix_socket(path, sock);
    }

    return false;
}

void unix_remove_unix_socket(const char path[], int sock_in)
{
    char *dir;

    dir = dirname(strdup(path));

    close(sock_in);
    unlink(path);
    rmdir(dir);
    free(dir);
}

void unix_fork(void)
{
    pid_t pid;
    pid_t sid;

    pid = fork();

    if (pid < 0) {
        log_error("Failed to fork: %s", strerror(errno));
        exit(1);
    } else if (pid != 0) {
        // Child process
        exit(0);
    }

    // Become session leader
    sid = setsid();
    if (sid < 0) {
        exit(1);
    }

    // Clear out the file mode creation mask
    umask(0);
}

void unix_write_pidfile(int pid, const char pidfile[])
{
    FILE *file;

    file = fopen(pidfile, "r");
    if (file) {
        fclose(file);
        log_error("PID file already exists: %s", pidfile);
        exit(1);
    }

    file = fopen(pidfile, "w");
    if (file == NULL) {
        log_error("Failed to open PID file: %s", strerror(errno));
        exit(1);
    }

    if (fprintf(file, "%i\n", pid) < 0) {
        log_error("Failed to write PID file: %s", strerror(errno));
        unlink(pidfile);
        exit(1);
    }

    if (fclose(file) < 0) {
        log_error("Failed to close PID file: %s", strerror(errno));
        unlink(pidfile);
        exit(1);
    }
}

void unix_dropuid0(void)
{
    struct passwd *pw;

    // Return if no user is set
    if (gconf->user == NULL) {
        return;
    }

    // Return if we are not root
    if (getuid() != 0) {
        return;
    }

    // Process is running as root, drop privileges
    if ((pw = getpwnam(gconf->user)) == NULL) {
        log_error("Dropping uid 0 failed. Set a valid user.");
        exit(1);
    }

    if (setenv("HOME", pw->pw_dir, 1) != 0) {
        log_error("Setting new $HOME failed.");
        exit(1);
    }

    if (setgid(pw->pw_gid) != 0) {
        log_error("Unable to drop group privileges");
        exit(1);
    }

    if (setuid(pw->pw_uid) != 0) {
        log_error("Unable to drop user privileges");
        exit(1);
    }

    // Test permissions
    if (setuid(0) != -1 || setgid(0) != -1) {
        log_error("We still have root privileges");
        exit(1);
    }
}
