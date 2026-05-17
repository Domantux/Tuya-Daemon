#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "app.h"
#include "daemon.h"
#include "config.h"

#define PID_FILE "/var/run/tuya_espd.pid"

/**
 * Signal handler for daemon shutdown
 */
static void signal_handler(int sig)
{
    const char *sig_name = "unknown";
    switch (sig) {
    case SIGINT:  sig_name = "SIGINT";  break;
    case SIGTERM: sig_name = "SIGTERM"; break;
    }
    syslog(LOG_SYSLOG_INFO, "Received signal %d (%s), shutting down gracefully",
           sig, sig_name);
    running = 0;
}

int daemon_check_and_write_pid(void)
{
    FILE *fp;
    pid_t running_pid;

    fp = fopen(PID_FILE, "r");
    if (fp) {
        if (fscanf(fp, "%d", &running_pid) == 1) {
            fclose(fp);
            if (kill(running_pid, 0) == 0) {
                syslog(LOG_SYSLOG_ERR,
                       "Another instance already running (PID %d)",
                       running_pid);
                return -1;
            } else {
                syslog(LOG_SYSLOG_INFO,
                       "Stale PID file (PID %d no longer running), cleaning up",
                       running_pid);
            }
        } else {
            syslog(LOG_SYSLOG_WARNING,
                   "PID file exists but is malformed, overwriting");
            fclose(fp);
        }
    }

    fp = fopen(PID_FILE, "w");
    if (!fp) {
        syslog(LOG_SYSLOG_WARNING, "Failed to create PID file %s: %s",
               PID_FILE, strerror(errno));
        return 0; /* Continue anyway */
    }

    fprintf(fp, "%d\n", getpid());
    fclose(fp);
    return 0;
}

void daemon_remove_pid(void)
{
    unlink(PID_FILE);
}

void daemon_rewrite_pid(void)
{
    FILE *fp = fopen(PID_FILE, "w");
    if (!fp) {
        syslog(LOG_SYSLOG_WARNING, "Failed to rewrite PID file: %s",
               strerror(errno));
        return;
    }
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
}

int daemon_daemonize(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_SYSLOG_ERR, "fork() failed: %s", strerror(errno));
        return -1;
    }
    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0) {
        syslog(LOG_SYSLOG_ERR, "setsid() failed: %s", strerror(errno));
        return -1;
    }

    /* Second fork to prevent acquiring a controlling terminal */
    pid = fork();
    if (pid < 0) {
        syslog(LOG_SYSLOG_ERR, "second fork() failed: %s", strerror(errno));
        return -1;
    }
    if (pid > 0)
        exit(EXIT_SUCCESS);

    chdir("/");
    umask(0);

    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO)
            close(devnull);
    }

    return 0;
}

void daemon_setup_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGHUP, SIG_IGN);
}
