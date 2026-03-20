#ifndef DAEMON_H
#define DAEMON_H

/**
 * Check if daemon is already running and write PID file
 * @return 0 on success, -1 if another instance is running
 */
int daemon_check_and_write_pid(void);

/**
 * Remove PID file on exit
 */
void daemon_remove_pid(void);

/**
 * Rewrite PID file with current process PID (after daemonize)
 */
void daemon_rewrite_pid(void);

/**
 * Fork and detach from terminal to run as daemon
 * @return 0 on success in child process, -1 on error
 */
int daemon_daemonize(void);

/**
 * Setup signal handlers for daemon (SIGINT, SIGTERM)
 */
void daemon_setup_signals(void);

#endif /* DAEMON_H */
