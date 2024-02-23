/**
 * @file UtilityLinux.cpp
 * @author Daniel Starke
 * @date 2024-02-15
 * @version 2024-02-23
 */
#include <pcf/Utility.hpp>


#ifdef PCF_IS_LINUX
#include <algorithm>
#include <FL/fl_ask.H>
#include <pcf/ScopeExit.hpp>
extern "C" {
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
}


#ifndef NSIG
#define NSIG 32
#endif
#undef READ_PIPE
#define READ_PIPE 0
#undef WRITE_PIPE
#define WRITE_PIPE 1


/**
 * Sets all password bytes to zero.
 * 
 * @param[out] buf - buffer containing the password
 * @param[in] len - number of bytes in `buf`
 */
static inline void emptyPassword(char * buf, const size_t len) {
	std::fill(const_cast<volatile char *>(buf), const_cast<volatile char *>(buf + len), 0);
}


/**
 * Ensures that the current process is running with root permissions.
 * This may open a password prompt and uses `sudo` as backend.
 *
 * @param[in] argc - number of command-line arguments
 * @param[in] argv - command-line arguments
 * @return true on success, else false
 */
bool requestRootPermission(int argc, char * argv[]) {
	const int euid = geteuid();
	if (euid == 0) return true;
	int pStandardInput[2] = {-1, -1};
	sigset_t oldMask, newMask;
	struct sigaction sigAction;
	bool hasOldMask = false;
	char arg0[] = "sudo";
	char arg1[] = "-S";
	char ** newArgv = NULL;
	const char * constPassword = NULL;
	size_t passwordLen;
	char * password = NULL;
	const auto closePipe = [](int & fd) {
		if (fd != -1) {
			close(fd);
			fd = -1;
		}
	};
	const auto cleanup = makeScopeExit([&]() {
		closePipe(pStandardInput[READ_PIPE]);
		closePipe(pStandardInput[WRITE_PIPE]);
		if (newArgv != NULL) free(newArgv);
		if (password != NULL) free(password);
	});

	fl_message_title("sudo vkvm");
	const struct passwd * pw = getpwuid(euid);
	if (pw != NULL) {
		constPassword = fl_password("Root permission is required. Enter password for \"%s\":", "", pw->pw_name);
	} else {
		constPassword = fl_password("Root permission is required. Please authenticate.", "");
	}
	if (constPassword == NULL) return false;
	passwordLen = strlen(constPassword);
	password = strdup(constPassword);
	emptyPassword(const_cast<char *>(constPassword), passwordLen);
	if (password == NULL) return false;

	/* build command-line for `sudo` call */
	newArgv = static_cast<char **>(calloc(1, sizeof(char *) * (argc + 3)));
	if (newArgv == NULL) return false;
	newArgv[0] = arg0;
	newArgv[1] = arg1;
	for (int i = 0; i < argc; i++) {
		newArgv[i + 2] = argv[i];
	}

	if (pipe(pStandardInput) != 0) goto onError;

	/* Temporary disable signal handling for calling thread to avoid unexpected signals
	 * between `fork()` and `execvp()`. */
	sigfillset(&newMask);
	if (pthread_sigmask(SIG_SETMASK, &newMask, &oldMask) != 0) goto onError;
	hasOldMask = true;

	switch (fork()) {
	case -1: /* fork failed */
		break;
	case 0: /* child */
		/* clear out signal handlers to avoid unexpected events */
		memset(&sigAction, 0, sizeof(sigAction));
		sigAction.sa_handler = SIG_DFL;
		sigemptyset(&sigAction.sa_mask);
		for (int i = 1; i < NSIG; i++) sigaction(i, &sigAction, NULL);
		if (pthread_sigmask(SIG_SETMASK, &oldMask, NULL) < 0) goto onError;

		/* forward password to parent process' standard input */
		closePipe(pStandardInput[READ_PIPE]);
		if (write(pStandardInput[WRITE_PIPE], password, strlen(password)) < 0) {
			emptyPassword(password, passwordLen);
			closePipe(pStandardInput[WRITE_PIPE]);
		} else {
			emptyPassword(password, passwordLen);
			closePipe(pStandardInput[WRITE_PIPE]);
			exit(EXIT_SUCCESS);
		}

		exit(EXIT_FAILURE);
		break;
	default: /* parent */
		/* restore original signal handler */
		if ( hasOldMask ) pthread_sigmask(SIG_SETMASK, &oldMask, NULL);

		/* set standard input accordingly */
		dup2(pStandardInput[READ_PIPE], STDIN_FILENO);

		/* close other pipe fd */
		closePipe(pStandardInput[WRITE_PIPE]);

		execvp(newArgv[0], newArgv);
		/* something went wrong */
		break;
	}
onError:
	/* restore original signal handler */
	if ( hasOldMask ) pthread_sigmask(SIG_SETMASK, &oldMask, NULL);
	return false;
}


#endif /* PCF_IS_LINUX */
