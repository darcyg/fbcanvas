#include <linux/input.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <fcntl.h>
#include <ncurses.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "keymap.h"

static bool need_repaint = false;

static void handle_signal(int s)
{
	int fd = open("/dev/tty", O_RDWR);
	if (fd)
	{
		if (s == SIGUSR1)
		{
			/* Release display */
			ioctl(fd, VT_RELDISP, 1);
		} else if (s == SIGUSR2) {
			/* Acquire display */
			ioctl(fd, VT_RELDISP, VT_ACKACQ);
			need_repaint = true;
		}

		close(fd);
	}
}

static int init_vt_switch(void)
{
	/* Asetetaan konsolinvaihto lähettämään signaaleja */
	int fd = open("/dev/tty", O_RDWR);
	if (fd >= 0)
	{
		struct vt_mode vt_mode;
		ioctl(fd, VT_GETMODE, &vt_mode);

		vt_mode.mode = VT_PROCESS; /* Tämä prosessi hoitaa vaihdot. */
		vt_mode.waitv = 0;
		vt_mode.relsig = SIGUSR1;
		vt_mode.acqsig = SIGUSR2;
		ioctl(fd, VT_SETMODE, &vt_mode);
		close(fd);

		signal(SIGUSR1, handle_signal);
		signal(SIGUSR2, handle_signal);

		return 0;
	}

	return -1;
}

static int fd = -1;
static struct pollfd pfd[2];
static sigset_t sigs;
static int modifiers = 0;

static int init_input(void)
{
	static const char kbd_dev[] = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
	sigfillset(&sigs);
	sigdelset(&sigs, SIGUSR1);
	sigdelset(&sigs, SIGUSR2);

	fd = open(kbd_dev, O_RDONLY);
	if (fd < 0)
	{
		perror(kbd_dev);
		return -1;
	}

	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;
	pfd[1].fd = fd;
	pfd[1].events = POLLIN;
	return 0;
}

int init_terminal(void)
{
	int ret;

	ret = init_vt_switch();
	if (ret < 0)
		goto out;
	ret = init_input();
out:
	return ret;
}

int read_key(void)
{
	for (;;)
	{
		int ret = ppoll(pfd, 2, NULL, &sigs);
		if (ret == -1) /* error, most likely EINTR. */
		{
			/* This will be handled later */
		} else if (ret == 0) { /* Timeout */
			/* Nothing to do */
		} else {
			/* evdev input available */
			if (pfd[1].revents)
			{
				struct input_event ev;
				read(fd, &ev, sizeof(ev));

				if (ev.type == EV_KEY)
				{
					unsigned int m = 0;
					if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT)
						m = SHIFT;
					if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
						m = CONTROL;
					if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
						m = ALT;

					if (ev.value == 1 || ev.value == 2)
						modifiers |= m;
					else if (ev.value == 0)
						modifiers &= ~m;
				}

				if (need_repaint)
				{
					need_repaint = false;
					return 'l' | CONTROL;
				}
			}

			/* stdin input available */
			if (pfd[0].revents)
			{
				int key = tolower(getch());

				/* CTRL-A ... CTRL-Z */
				if (key >= 1 && key <= 26)
					key += 'a' - 1;

				return modifiers | key;
			}
		}
	}
}
