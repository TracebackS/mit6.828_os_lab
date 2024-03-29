// Stripped-down primitive printf-style formatting routines,
// used in common by printf, sprintf, fprintf, etc.
// This code is also used by both the kernel and user programs.

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/stdarg.h>
#include <inc/error.h>

/*
 * Space or zero padding and a field width are supported for the numeric
 * formats only.
 *
 * The special format %e takes an integer error code
 * and prints a string describing the error.
 * The integer may be positive or negative,
 * so that -E_NO_MEM and E_NO_MEM are equivalent.
 */

static const char * const error_string[MAXERROR] =
{
	[E_UNSPECIFIED]	= "unspecified error",
	[E_BAD_ENV]	= "bad environment",
	[E_INVAL]	= "invalid parameter",
	[E_NO_MEM]	= "out of memory",
	[E_NO_FREE_ENV]	= "out of environments",
	[E_FAULT]	= "segmentation fault",
};


static void printstring(void(*putch)(int, void*), void *putdat, const char* s) {
	while (*s) {
		putch(*s++, putdat);
	}
}

/*
 * Print a number (base <= 16) in reverse order,
 * using specified putch function and associated pointer putdat.
 */
static int printnum(void (*putch)(int, void*), void *putdat,
	 unsigned long long num, unsigned base, int width, int padc, int laflag)
{
	// if cprintf'parameter includes pattern of the form "%-", padding
	// space on the right side if neccesary.
	// you can add helper function if needed.

	unsigned char digits[sizeof(unsigned long long) * 8] = {0};
	int len = 0;
	unsigned long long n = num; 

	do {
		digits[len++] = n % base;
		n /= base;
	} while (n);

	int npad = width - len;
	if (!laflag)
	{
		while (npad-- > 0)
		{
			putch(padc, putdat);
		}
	}

	int i;
	for (i = len-1; i >= 0; --i) {
		putch("0123456789abcdef"[digits[i]], putdat);
	}

	if (laflag)
	{
		while (npad-- > 0)
		{
			putch(padc, putdat);
		}
	}

/*
	// first recursively print all preceding (more significant) digits
	int ret = 0;
	if (num >= base) {
		ret = printnum(putch, putdat, num / base, base, width - 1, padc);
	} else {
		// print any needed pad characters before first digit
		while (--width > 0) {
			putch(padc, putdat);
			ret++;
		}
	}

	// then print this (the least significant) digit
	return ret + 1;
*/
	return width > len ? width : len;
}

// Get an unsigned int of various possible sizes from a varargs list,
// depending on the lflag parameter.
static unsigned long long
getuint(va_list *ap, int lflag)
{
	if (lflag >= 2)
		return va_arg(*ap, unsigned long long);
	else if (lflag)
		return va_arg(*ap, unsigned long);
	else
		return va_arg(*ap, unsigned int);
}

// Same as getuint but signed - can't use getuint
// because of sign extension
static long long
getint(va_list *ap, int lflag)
{
	if (lflag >= 2)
		return va_arg(*ap, long long);
	else if (lflag)
		return va_arg(*ap, long);
	else
		return va_arg(*ap, int);
}


void my_putch(void (*putch)(int, void*), void *putdat, char ch, int* counter)
{
	putch(ch, putdat);
	counter[0]++;
}


// Main function to format and print a string.
void printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);

void
vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list ap)
{
	register const char *p;
	register int ch, err;
	unsigned long long num;
	int base, lflag, width, precision, altflag, dsplflag, laflag;
	char padc;

	int n_char_put = 0;

	// printstring(putch, putdat, "DEBUG: fmt='");
	// printstring(putch, putdat, fmt);
	// printstring(putch, putdat, "'\n");

	while (1) {
		while ((ch = *(unsigned char *) fmt++) != '%') {
			if (ch == '\0') {
				// printstring(putch, putdat, "\nDEBUG: n_char_put=");
				// printnum(putch, putdat, n_char_put, 10, -1, ' ');
				// printstring(putch, putdat, "\n");
				return;
			}
			my_putch(putch, putdat, ch, &n_char_put);
		}

		// Process a %-escape sequence
		padc = ' ';
		width = -1;
		precision = -1;
		lflag = 0;
		altflag = 0;
		dsplflag = 0;
		laflag = 0;
	reswitch:
		switch (ch = *(unsigned char *) fmt++) {
		case '+':
			dsplflag = 1;
			goto reswitch;

		// flag to pad on the right
		case '-':
			laflag = 1;
			goto reswitch;

		// flag to pad with 0's instead of spaces
		case '0':
			padc = '0';
			goto reswitch;

		// width field
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			for (precision = 0; ; ++fmt) {
				precision = precision * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9')
					break;
			}
			goto process_precision;

		case '*':
			precision = va_arg(ap, int);
			goto process_precision;

		case '.':
			if (width < 0)
				width = 0;
			goto reswitch;

		case '#':
			altflag = 1;
			goto reswitch;

		process_precision:
			if (width < 0)
				width = precision, precision = -1;
			goto reswitch;

		// long flag (doubled for long long)
		case 'l':
			lflag++;
			goto reswitch;

		// character
		case 'c':
			my_putch(putch, putdat, va_arg(ap, int), &n_char_put);
			break;

		// error message
		case 'e':
			err = va_arg(ap, int);
			if (err < 0)
				err = -err;
			if (err >= MAXERROR || (p = error_string[err]) == NULL)
				printfmt(putch, putdat, "error %d", err);
			else
				printfmt(putch, putdat, "%s", p);
			break;

		// string
		case 's':
			if ((p = va_arg(ap, char *)) == NULL)
				p = "(null)";
			if (width > 0 && padc != '-')
				for (width -= strnlen(p, precision); width > 0; width--)
					my_putch(putch, putdat, padc, &n_char_put);
			for (; (ch = *p++) != '\0' && (precision < 0 || --precision >= 0); width--)
				if (altflag && (ch < ' ' || ch > '~'))
					my_putch(putch, putdat, '?', &n_char_put);
				else
					my_putch(putch, putdat, ch, &n_char_put);
			for (; width > 0; width--)
				my_putch(putch, putdat, ' ', &n_char_put);
			break;

		// (signed) decimal
		case 'd':
			num = getint(&ap, lflag);
			base = 10;
			goto number;

		// unsigned decimal
		case 'u':
			num = getuint(&ap, lflag);
			base = 10;
			goto number;

		// (unsigned) octal
		case 'o':
			// Replace this with your code.
			num = getint(&ap, lflag);
			my_putch(putch, putdat, '0', &n_char_put);
			base = 8;
			goto number;

		// pointer
		case 'p':
			my_putch(putch, putdat, '0', &n_char_put);
			my_putch(putch, putdat, 'x', &n_char_put);
			num = (unsigned long long)
				(uintptr_t) va_arg(ap, void *);
			base = 16;
			goto number;

		// (unsigned) hexadecimal
		case 'x':
			num = getuint(&ap, lflag);
			base = 16;
		number:
			if ((long long) num < 0) {
				my_putch(putch, putdat, '-', &n_char_put);
				num = -(long long) num;
			}
			else if (num >= 0 && dsplflag)
			{
				my_putch(putch, putdat, '+', &n_char_put);
			}
			n_char_put += printnum(putch, putdat, num, base, width, padc, laflag);
			break;

		case 'n': {
				  // You can consult the %n specifier specification of the C99 printf function
				  // for your reference by typing "man 3 printf" on the console. 

				  // 
				  // Requirements:
				  // Nothing printed. The argument must be a pointer to a signed char, 
				  // where the number of characters written so far is stored.
				  //

				  // hint:  use the following strings to display the error messages 
				  //        when the cprintf function ecounters the specific cases,
				  //        for example, when the argument pointer is NULL
				  //        or when the number of characters written so far 
				  //        is beyond the range of the integers the signed char type 
				  //        can represent.

				  const char *null_error = "\nerror! writing through NULL pointer! (%n argument)\n";
				  const char *overflow_error = "\nwarning! The value %n argument pointed to has been overflowed!\n";
				  signed char* p = va_arg(ap, signed char*);
				  if (n_char_put > 127) {
					  printstring(putch, putdat, overflow_error);
				  }
				  if (!p) {
				      printstring(putch, putdat, null_error);
				  } else {
					  *p = n_char_put;
				  }
				  break;
			  }

		// escaped '%' character
		case '%':
			my_putch(putch, putdat, ch, &n_char_put);
			break;

		// unrecognized escape sequence - just print it literally
		default:
			my_putch(putch, putdat, '%', &n_char_put);
			for (fmt--; fmt[-1] != '%'; fmt--)
				/* do nothing */;
			break;
		}
	}
}

void
printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintfmt(putch, putdat, fmt, ap);
	va_end(ap);
}

struct sprintbuf {
	char *buf;
	char *ebuf;
	int cnt;
};

static void
sprintputch(int ch, struct sprintbuf *b)
{
	b->cnt++;
	if (b->buf < b->ebuf)
		*b->buf++ = ch;
}

int
vsnprintf(char *buf, int n, const char *fmt, va_list ap)
{
	struct sprintbuf b = {buf, buf+n-1, 0};

	if (buf == NULL || n < 1)
		return -E_INVAL;

	// print the string to the buffer
	vprintfmt((void*)sprintputch, &b, fmt, ap);

	// null terminate the buffer
	*b.buf = '\0';

	return b.cnt;
}

int
snprintf(char *buf, int n, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vsnprintf(buf, n, fmt, ap);
	va_end(ap);

	return rc;
}


