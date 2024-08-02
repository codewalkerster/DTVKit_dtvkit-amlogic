#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

/*
 * Read ccextractor's output (in rcwt format) from stdin,
 * remove header and FTS, output the raw cc data to stdout.
 */

/*
 * RCWT (Raw Captions With Time), version 0.001
============================================
File header (11 bytes), required:

byte(s)   value   description
0-2       CCCCED  magic number, for Closed Caption CC Extractor Data
3         CC      Creating program.  Currently Defined:
                       CC -> CCextractor.
4-5       0052    Program version number
6-7       0001    File format version
8-10      000000  Reserved
\r\n

Time header, required for every group of data packets
0-7       <var>   FTS value (Time in ms)
\r\n
8-9       <var>   (1-65535) Number of caption data blocks with this time
                 stamp.  If the number of blocks exceeds 65535 write
                 another time header for the remaining blocks.
\r\n

Caption data
0-2       <var>   Three byte caption data.
\r\n

 */

#define err(_fmt, ...) fprintf(stderr, _fmt"\n", ##__VA_ARGS__)
#define inf(_fmt, ...) fprintf(stderr, _fmt"\n", ##__VA_ARGS__)

static uint64_t pos;

static int readn(int fd, void *buf, size_t n)
{
    int ret = 0;
    int left = n;
    while (1) {
        ret = read(fd, buf + (n - left), left);
        if (ret == left)
            return n;
        else if (ret <= 0) {
            if (errno == EAGAIN)
                continue;
            else {
                err("read fail: err:%d/%s", errno, strerror(errno));
                return -1;
            }
        } else if (ret < left) {
            left -= ret;
        } else
            return -1;
    }
}
static int write_n(int fd, void *buf, size_t n)
{
    int ret = 0;
    int left = n;
    while (1) {
        ret = write(fd, buf + (n - left), left);
        if (ret == left)
            return left;
        else if (ret <= 0) {
            if (errno == EAGAIN)
                continue;
            else
                return -1;
        } else if (ret < left) {
            left -= ret;
        } else
            return -1;
    }
}

static int get_tl(int fd, int *t, size_t *l)
{
    char buf[10];

    int ret = readn(fd, buf, 1);
    pos++;
    if (ret != 1)
        return -1;

    int cmd = buf[0];
    //inf("t:%d = %d @%lx", cmd, buf[0], pos);

    ret = readn(fd, buf, 10);
    pos += 10;
    if (ret != 10)
        return -1;

    size_t size;
    ret = sscanf(buf, "%zu", &size);

    *t = cmd;
    *l = size;

    return 0;
}

/*
 * 1:  skipped,
 * 0:  need further process
 * -1: error
 */
static int try_skip (int fd, int *pt, size_t *pl)
{
    int t;
    size_t l;
    char buf[2];

    if (get_tl(fd, &t, &l) != 0) {
        err("get tl fail.");
        return -1;
    }

    switch (t) {
        case 0x6:/*DATA*/
            *pt = t;
            *pl = l;
            return 0;

        case 55:/*PING*/ {

            if (readn(fd, buf, 2) != 2) {//\r\n
                err("read != 2");
                return -1;
            }
            pos += 2;

            inf("PING reply");
            char cmd = 55;
            if (write_n(fd, &cmd, 1) != 1) {
                err("fail to resp ping.");
                return -1;
            }
            } break;

        default:
            /*drop*/
            inf("skip t:%d, l;%zd", t, l);
            while (l--) {
                if (readn(fd, buf, 1) != 1)
                    return -1;
                pos++;
            }
            if (readn(fd, buf, 2) != 2) {//\r\n
                err("read != 2");
                return -1;
            }
            pos += 2;
            break;
    }
    return 1;
}

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    unsigned char buf[10];

    int t;
    size_t l;

    int fd = -1;

    if (argc < 2) {
        err("Usage: %s <port>", argv[0]);
        return -1;
    }

    int socket_port = 33333;

    sscanf(argv[1], "%i", &socket_port);
    inf("port: %d", socket_port);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err("open socket fail");
        return -1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET,
                SO_REUSEADDR | SO_REUSEPORT,
                &opt, sizeof(opt))) {
        err("setsockopt fail");
        return -1;
    }

    struct sockaddr_in s_in;

    memset(&s_in, 0, sizeof(s_in));
    s_in.sin_family = AF_INET;
    s_in.sin_addr.s_addr = INADDR_ANY;
    s_in.sin_port = htons(socket_port);

    if (bind(fd, (struct sockaddr *)&s_in, sizeof(s_in)) < 0) {
        err("bind fail");
        return -1;
    }

    inf("socket ready.");

    if (listen(fd, 1) < 0) {
        err("listen fail");
        return -1;
    }

    struct sockaddr_un c_un;
    socklen_t c_len = sizeof(c_un);
    int connfd;

    if ((connfd = accept(fd, (struct sockaddr *)&c_un, &c_len)) < 0) {
        err("accept error, err(%d:%s)", errno, strerror(errno));
        return -1;
    }


    do {

        while (1) {
            int ret = try_skip(connfd, &t, &l);
            if (ret == 0)
                break;
            else if (ret == -1)
                return -1;
            else
                continue;
        }

        if (t != 0x6) {
            err("unknown tag :%d", t);
            return -1;
        }

        if (l != 10) {
            err("%zd != 10, incorrect header, pos:%"PRIx64, l, pos);
            return -1;
        }

        if (readn(connfd, buf, 10) != 10) {
            err("read 10 fail");
            return -1;
        }
        pos += 10;

        /*0:7=fts 8:9=cbcount*/
        //uint64_t fts = *((uint64_t*)buf);
        int cbcount = *((uint16_t*)(buf + 8));

        //inf("fts: %lx, cbcount: %d", fts, cbcount);

        if (readn(connfd, buf, 2) != 2) {/*\r\n*/
            err("read != 2");
            return -1;
        }
        pos += 2;

        /*cc data*/
        if (get_tl(connfd, &t, &l) != 0)
            return -1;

        if (t != 0x6 || l != (cbcount * 3)) {
            err("bad tag or bad data size: tag:%02x, l:%zd, @%"PRIx64, t, l, pos);
            return -1;
        }

        while (cbcount--) {
            if (readn(connfd, buf, 3) != 3)
                return -1;
            pos += 3;

            write_n(1, buf, 3);
        }

        if (readn(connfd, buf, 2) != 2) {/*\r\n*/
            err("read != 2");
            return -1;
        }
        pos += 2;

    } while (1);

    return 0;
}


