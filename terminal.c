#include <linux/input.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "document.h"
#include "keymap.h"

static bool active_console = true;
static bool need_repaint = false;

static void handle_signal(int s)
{
	int fd = open("/dev/tty", O_RDWR);
	if (fd)
	{
		if (s == SIGUSR1)
		{
			/* Release display */
			active_console = false;
			ioctl(fd, VT_RELDISP, 1);
		} else if (s == SIGUSR2) {
			/* Acquire display */
			ioctl(fd, VT_RELDISP, VT_ACKACQ);
			need_repaint = true;
			active_console = true;
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

static int keyboard_fd = -1;
static int mouse_fd = -1;
static struct pollfd pfd[2];
static sigset_t sigs;
static int modifiers = 0;

static int init_input(void)
{
	static const char kbd_dev[] = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
	static const char mouse_dev[] = "/dev/input/by-path/platform-i8042-serio-1-event-mouse";
	sigfillset(&sigs);
	sigdelset(&sigs, SIGUSR1);
	sigdelset(&sigs, SIGUSR2);

	keyboard_fd = open(kbd_dev, O_RDONLY);
	if (keyboard_fd < 0)
	{
		perror(kbd_dev);
		return -1;
	}

	mouse_fd = open(mouse_dev, O_RDONLY);
	if (mouse_fd < 0)
		perror(mouse_dev);

	pfd[0].fd = mouse_fd;
	pfd[0].events = POLLIN;
	pfd[1].fd = keyboard_fd;
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

int read_key(struct document *doc)
{
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 50000000};
	struct timespec *idletimer = NULL;

	if (doc->backend->idle_callback)
		idletimer = &ts;

	for (;;)
	{
		struct input_event ev;
		int ret = ppoll(pfd, 2, idletimer, &sigs);
		if (ret == -1) /* error, most likely EINTR. */
		{
			/* This will be handled later */
			continue;
		}

		if (ret == 0) /* Timeout */
		{
			/* Call backend-specific idle function */
			if (doc->backend->idle_callback)
			{
				doc->flags |= DOCUMENT_IDLE;
				doc->backend->idle_callback(doc);
				doc->flags &= ~DOCUMENT_IDLE;
			}
			continue;
		}

		if (pfd[0].revents) /* Mouse input available */
			read(pfd[0].fd, &ev, sizeof(ev));
		else if (pfd[1].revents) /* Keyboard input available */
			read(pfd[1].fd, &ev, sizeof(ev));
		else
			continue;

		switch (ev.type)
		{
#if 0
		case EV_REL:
		{
			if (ev.code == REL_X)
			{
				if (ev.value < 0)
					return KEY_LEFT | SHIFT;
				else if (ev.value > 0)
					return KEY_RIGHT | SHIFT;
			} else if (ev.code == REL_Y) {
				if (ev.value < 0)
					return KEY_UP | SHIFT;
				else if (ev.value > 0)
					return KEY_DOWN | SHIFT;
			}

			break;
		}
#endif
		case EV_KEY:
		{
			int key = 0;
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

			if (m)
				break;

			if (ev.value == 1 || ev.value == 2)
			{
				/* Ei päästetä ALT+konsolinvaihtoa pääohjelmaan asti. */
				switch (ev.code)
				{
					case KEY_F1 ... KEY_F10:
					case KEY_F11 ... KEY_F12:
					case KEY_LEFT:
					case KEY_RIGHT:
						if (!(modifiers & ALT))
							key = ev.code;
						break;
					default:
						key = ev.code;
						break;
				}
			}

			if (key && active_console)
				return modifiers | key;
			break;
		}

		default:
			break;
		}

		if (need_repaint)
		{
			need_repaint = false;
			return KEY_L | CONTROL;
		}
	}
}
