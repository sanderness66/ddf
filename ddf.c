// DDF.C -- bit like an ancient df
//
// svm 23-APR-2022 - 26-FEB-2026
//

#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/types.h>

#define MPLEN 12

static int full = 0;
const char *fmtp = "%-*s %-8s\n";
const char *fmtf = "%-*s %-*s %-8s %-8s %-8s %-8s %6s\n";
const char *tavoid[] = { "tmpfs", "devtmpfs", "rootfs", "squashfs", "overlay", "fuse.glusterfs", "efivarfs", "fuse.sshfs", NULL };
const char *mavoid[] = { "/var/snap/", "/var/lib/docker", NULL };

char *
human (long i, int bl)
{
    char *buf = malloc (32);
    long kb, tkb;

    kb = (i * bl) / 1024;
    if (kb < 1024) {
	tkb = 10 * kb;
	sprintf (buf, "%5.1fk", (float) tkb / 10);
    } else if (kb < (1024 * 1024)) {
	tkb = 10 * kb / 1024;
	sprintf (buf, "%5.1fM", (float) tkb / 10);
    } else if (kb < (1024 * 1024 * 1024)) {
	tkb = 10 * kb / (1024 * 1024);
	sprintf (buf, "%5.1fG", (float) tkb / 10);
    } else {
	tkb = 10 * kb / (1024 * 1024 * 1024);
	sprintf (buf, "%5.1fT", (float) tkb / 10);
    }
    return buf;
}

char *
pct (long tot, long avl)
{
    char *buf = malloc (32);
    long used = tot - avl;
    float p = ((100 * (float) used) / (float) tot);

    sprintf (buf, "%2.1f%% ", p);
    return buf;
}

char *
base (char *s)
{
    char *p, *q;

    for (p = q = s; *p; p++)
	if (*p == '/')
	    q = p;

    if (*q == '/')
	q++;
    return (q);
}

char *
typ (char *s)
{
    char *p = strchr (s, '.');

    if (p)
	return (p + 1);
    else
	return s;
}

#define maybe_base(s) (strlen(s) >= MPLEN ? base(s) : s)

void
prmnt (struct mntent *m)
{
    struct statvfs s;

    if (statvfs (m->mnt_dir, &s) == 0)
	if (s.f_blocks > 0)
	    if (full)
		printf (fmtf, MPLEN, maybe_base (m->mnt_dir), MPLEN, base (m->mnt_fsname),
			typ (m->mnt_type), human (s.f_blocks, s.f_bsize),
			human (s.f_blocks - s.f_bfree, s.f_bsize),
			human (s.f_bfree, s.f_bsize), pct (s.f_blocks, s.f_bfree));
	    else
		printf (fmtp, MPLEN, maybe_base (m->mnt_dir), human (s.f_bfree, s.f_bsize));
}

int
main (int ac, char **av)
{
    FILE *fp;
    struct mntent *m;

    if (ac > 1 && av[1][0] == '-') {
	switch (av[1][1]) {
	case 'f':
	    full = 1;
	    ac--;
	    *av++;
	    break;
	default:
	    puts ("bad arg");
	    exit (1);
	}
    }

    if (full)
	printf (fmtf, MPLEN, "dir", MPLEN, "dev", "type", "  size", "  used", "  free", "%use ");

    if ((fp = setmntent ("/etc/mtab", "r")) == NULL) {
	perror ("ddf: /etc/mtab");
	exit (1);
    }
    while ((m = getmntent (fp))) {
	if (ac > 1) {
	    int i;

	    for (i = 1; i < ac; i++) {
		struct stat st1, st2;

		stat (av[i], &st1);
		stat (m->mnt_dir, &st2);

		if (S_ISBLK (st1.st_mode)) {
		    if (st1.st_rdev == st2.st_dev) {
			prmnt (m);
		    }
		} else {
		    if (st1.st_dev == st2.st_dev)
			prmnt (m);
		}
	    }
	} else {
	    int i;

	    for (i = 0; tavoid[i]; i++)
		if (strcmp (tavoid[i], m->mnt_type) == 0)
		    goto end;
	    for (i = 0; mavoid[i]; i++)
		if (strncmp (mavoid[i], m->mnt_dir, strlen (mavoid[i])) == 0)
		    goto end;

	    prmnt (m);
end:
	    ;
	}
    }
    endmntent (fp);
    exit (0);
}
