
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include <arpa/inet.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void append(char **pp, char *end, const char *s) {
  size_t slen = strlen(s);
  if (*pp + slen > end)
    slen = end-*pp;
  memcpy(*pp, s, slen);
  *pp += slen;
}

static void append_uptime(char **pp, char *end, long uptime) {
  if (*pp < end) {
    ldiv_t s = ldiv(uptime, 60);
    ldiv_t m = ldiv(s.quot, 60);
    ldiv_t h = ldiv(m.quot, 24);
    int len = snprintf(*pp, end-*pp, " up %ld-%02ld:%02ld:%02ld", h.quot, h.rem, m.rem, s.rem);
    if (*pp + len <= end)
      *pp += len;
    else
      *pp = end;
  }
}

static void append_load(char **pp, char *end, double *loadavg) {
  if (*pp < end) {
    int len;
    len = snprintf(*pp, end-*pp, " load %.2f %.2f %.2f", loadavg[0], loadavg[1], loadavg[2]);
    if (*pp + len <= end) {
      *pp += len;
    } else {
      *pp = end;
    }
  }
}

static void append_firstline(char **pp, char *end, const char *filename) {
  if (*pp + 2 < end) {
    int fd;
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
      perror(filename);
      return;
    }
    **pp = ' ';
    char *ptr = *pp + 1;
    ssize_t len;
    len = read(fd, ptr, end-ptr);
    if (len == -1)
      perror("read");
    int err;
    err = close(fd);
    if (err == -1)
      perror("close");
    if (len < 1)
      return;
    char *eol;
    eol = memchr(ptr, len, '\n');
    if (eol)
      *pp = eol;
    else
      *pp = ptr+len;
  }
}

int main(int argc, char **argv) {

  const char *quotefile = "/etc/quote";
  int port = 17;
  
  int opt;
  while ((opt = getopt(argc, argv, "f:p:h")) != -1) {
    switch (opt) {
    case 'f':
      quotefile = optarg;
      break;
    case 'p':
      port = atoi(optarg);
      break;
    default:
      fprintf(stderr, "usage: %s [-f quotefile] [-p port]\n", argv[0]);
      exit(1);
    }
  }

  int fd;
  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd == -1) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  
  int err;
  err = bind(fd, (struct sockaddr *)&sin, sizeof(sin));
  if (err == -1) {
    perror("bind");
    exit(1);
  }
  
  char buf[512];
  ssize_t len, res;
  struct sockaddr_in from;
  socklen_t fromlen;
  char fromstr[INET_ADDRSTRLEN];
  struct utsname uts;
  struct sysinfo sinfo;
  double loadavg[3];
  
  printf("Ready.\n");
  for (;;) {
    fromlen = sizeof(from);
    len = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
    inet_ntop(AF_INET, &from.sin_addr, fromstr, sizeof(fromstr));
    printf("Received %zd byte(s) from %s:%d\n", len, fromstr, ntohs(from.sin_port));

    err = uname(&uts);
    if (err == -1) {
      perror("uname");
      continue;
    }

    err = sysinfo(&sinfo);
    if (err == -1) {
      perror("sysinfo");
      continue;
    }

    err = getloadavg(loadavg, 3);
    if (err != 3) {
      perror("getloadavg");
      continue;
    }

    char *ptr = buf;
    char *end = buf + sizeof(buf);
    append(&ptr, end, uts.nodename);
    append_uptime(&ptr, end, sinfo.uptime);
    append_load(&ptr, end, loadavg);
    append_firstline(&ptr, end, quotefile);

    len = ptr - buf;
    res = sendto(fd, buf, len, 0, (struct sockaddr *)&from, sizeof(from));
    if (res != len) {
      perror("sendto");
    }
  }
}
