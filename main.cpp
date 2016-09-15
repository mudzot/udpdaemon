#include <getopt.h>
#include <sys/stat.h>
#include "UdpDaemon.h"

bool goDaemon();

int main(int argc, char** argv)
{
	int c;
	DaemonConfig cfg;
	bool runAsDaemon = false;

	/* Get command line params */
	while ((c = getopt(argc, argv, "bda:l:")) != -1) {
		switch (c) {
			case 'a':
				cfg.bindAddr = optarg;
				break;
			case 'b':
				cfg.receiveBroadcast = true;
				break;
			case 'd':
				runAsDaemon = true;
				break;
			case 'l':
				cfg.port = optarg;
				break;
            default:
				fprintf(stderr, "Unknown option \n");
                return 1;
        }
    }
    argc = argc - optind;
    argv += optind;

	if (runAsDaemon && goDaemon()) {
		printf("Go daemon successfully, return\n");
		return 0;
	}

	UdpDaemon d(cfg);
	if (d.bindAddr()) {
		return 1;
	}
	d.run();
	return 0;
}

bool goDaemon() {
	pid_t pid = fork();
	if (pid < 0) {
		//Fork failed, fall back to normal mode
		return false;
	}
	if (pid > 0) {
		return true;
	} else {
		setsid();
		umask(0);
		int rc = chdir("/");
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		return false;
	}
}
