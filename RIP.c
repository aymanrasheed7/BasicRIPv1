#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<pthread.h>
#ifdef _WIN32
#include<Winsock2.h>
#include<Ws2tcpip.h>
#include<Windows.h>
#else
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<unistd.h>
#include<time.h>
#endif
#define RTMSIZE 64
#define UDPPORT 520
#define BUFSIZE 1024
#define CASTADR "224.0.0.9"
typedef struct {
    u_int dest, mask, gate, itfs, metr;
    int timer;
}Route;
Route rTable[RTMSIZE], nTable[RTMSIZE];
char buf0[BUFSIZE], buf1[BUFSIZE];
int prtm, nSize, mSize, rtSize;
u_int ipToInt(char* str) {
    u_int a, b, c, d;
    return sscanf(str, "%u.%u.%u.%u", &a, &b, &c, &d),
        a << 24 | b << 16 | c << 8 | d;
}
u_int chToInt(char* str) {
    return str[0] << 24 | str[1] << 16 | str[2] << 8 | str[3];
}
u_int chkNbr(int nBytes) {
    for (int i = nBytes / 20; i--;) if (!buf1[23 + 20 * i]) for (int j = nSize;
        j--;) if (chToInt(buf1 + 8 + 20 * i) == nTable[j].dest) return j;
    return -1;
}
u_int cmpStr(char* src, char* dst, int len) {
    for (int i = len; i--;) if (src[i] != dst[i]) return 1;
    return 0;
}
void ipPrint(u_int ipa, char end) {
    printf("%u.%u.%u.%u%c", ipa >> 24 & 255,
        ipa >> 16 & 255, ipa >> 8 & 255, ipa & 255, end);
}
void rtPrint() {
    printf("Network Destination\tNetmask\tGateway\tInterface\t");
    printf("Metric\tTimer\n");
    for (int i = rtSize; i--; ipPrint(rTable[i].dest, '\t'),
        ipPrint(rTable[i].mask, '\t'), ipPrint(rTable[i].gate, '\t'),
        ipPrint(rTable[i].itfs, '\t'), printf("%u\t%d\n",
            rTable[i].metr, rTable[i].timer - time(0)));
}
void rtSend() {
    printf("Routing table before sending:\n"), rtPrint();
    for (int i = (mSize = 4 + 20 * rtSize, buf0[0] = 2, buf0[1] = 1, rtSize);
        i--; buf0[8 + 20 * i] = rTable[i].dest >> 24,
        buf0[9 + 20 * i] = rTable[i].dest >> 16,
        buf0[10 + 20 * i] = rTable[i].dest >> 8,
        buf0[11 + 20 * i] = rTable[i].dest,
        buf0[23 + 20 * i] = rTable[i].metr, buf0[i * 20 + 5] = 2);
}
void rtRecv(int nBytes, int nId) {
    for (u_int i = nBytes / 20, j, k, l; i--;) {
        for (j = 0, k = chToInt(buf1 + 8 + 20 * i), l = buf1[23 + 20 * i];
            j < rtSize && k != rTable[j].dest; ++j);
        if (k == nTable[nId].dest) {
            if (j == rtSize) rTable[rtSize++] = nTable[nId];
            rTable[j].metr = 0, rTable[j].timer = 180 + time(0);
        } else if (l < 16) {
            if (j == rtSize) rTable[rtSize++] = nTable[nId], rTable[j].dest = k,
                rTable[j].mask = -1, rTable[j].metr = 16,
                rTable[j].timer = 120 + time(0);
            if (1 + l <= rTable[j].metr) rTable[j].metr = l + 1, rTable[j].gate
                = nTable[nId].gate, rTable[j].itfs = nTable[nId].itfs,
                rTable[j].timer = 180 + time(0);
        } else if (j < rtSize && rTable[j].metr < 16)
            rTable[j].metr = 16, rTable[j].timer = 120 + time(0);
    } printf("Routing table after receiving:\n"), rtPrint();
}
void rtBrush() {
    for (int i = rtSize; i--;)
        if (rTable[i].metr < 16 && rTable[i].timer < time(0))
            rTable[i].metr = 16, rTable[i].timer = 120 + time(0);
        else if (rTable[i].metr >= 16 && rTable[i].timer < time(0))
            rTable[i] = rTable[--rtSize];
}
void* broadCast() {
#ifdef _WIN32
    WSADATA lpWSAData;
    assert(~WSAStartup(0x1010, &lpWSAData));
#endif
    struct sockaddr_in to;
    int soc = socket(AF_INET, SOCK_DGRAM, 0);
    assert(soc >= 0), srand(soc ^ time(0)), prtm = 25 + rand() % 11 + time(0);
    memset(&to, 0, sizeof(to)), to.sin_addr.s_addr = inet_addr(CASTADR);
    to.sin_family = AF_INET, to.sin_port = htons(UDPPORT);
    for (; 1;) {
        if (mSize) assert(sendto(soc, buf0, mSize, 0,
            (struct sockaddr*)&to, sizeof(to)) >= 0), mSize = 0;
        if (prtm < time(0)) prtm = 25 + rand() % 11 + time(0), rtSend();
    }
}
void* listenPkt() {
#ifdef _WIN32
    WSADATA lpWSAData;
    assert(~WSAStartup(0x1010, &lpWSAData));
#endif
    u_int opt1 = 1;
    struct ip_mreq opt2;
    struct sockaddr_in from;
    int soc = socket(AF_INET, SOCK_DGRAM, 0);
    assert(soc >= 0), assert(setsockopt(soc, SOL_SOCKET, SO_REUSEADDR,
        (char*)&opt1, sizeof(opt1)) >= 0);
    memset(&from, 0, sizeof(from)), from.sin_addr.s_addr = htonl(INADDR_ANY);
    from.sin_family = AF_INET, from.sin_port = htons(UDPPORT);
    assert(bind(soc, (struct sockaddr*)&from, sizeof(from)) >= 0);
    opt2.imr_interface.s_addr = htonl(INADDR_ANY);
    opt2.imr_multiaddr.s_addr = inet_addr(CASTADR);
    assert(setsockopt(soc, IPPROTO_IP, IP_ADD_MEMBERSHIP,
        (char*)&opt2, sizeof(opt2)) >= 0);
    for (; 1; rtBrush()) {
        int fromlen = sizeof(from), nBytes, nId;
        assert((nBytes = recvfrom(soc, buf1, BUFSIZE, 0,
            (struct sockaddr*)&from, &fromlen)) >= 0);
        if (nBytes && buf1[0] == 1) rtSend();
        else if (nBytes && cmpStr(buf0, buf1, nBytes)
            && ~(nId = chkNbr(nBytes))) rtRecv(nBytes, nId);
    }
}
int main(int argc, char* argv[]) {
    FILE* confFile = fopen(argv[argc - 1], "r");
    for (; ~fscanf(confFile, "%*d%s%s", buf0, buf1); nTable[nSize] =
        (Route){ .mask = -1, .metr = 0, .timer = 180 + time(0) },
        nTable[nSize].dest = nTable[nSize].gate = ipToInt(buf1),
        nTable[nSize].dest &= nTable[nSize].mask &= ~(nTable[nSize].dest
            ^ (nTable[nSize].itfs = ipToInt(buf0))), ++nSize);
    fclose(confFile), memset(buf0, 0, sizeof(buf0));
    buf0[0] = buf0[1] = 1, buf0[5] = 2, mSize = 24;
    for (int i = nSize; i--;) for (int j = nSize; j--;)
        if (nTable[i].itfs == nTable[j].itfs) nTable[i].dest &=
            nTable[i].mask &= ~(nTable[i].dest ^ nTable[j].dest);
    for (int i = rtSize = nSize; i--; rTable[i] = nTable[i]) for (u_int
        u = nTable[i].mask; u + (u & -u); nTable[i].mask = u ^= u & -u);
    pthread_t thrB, thrL;
    pthread_create(&thrB, 0, broadCast, 0);
    pthread_create(&thrL, 0, listenPkt, 0);
    pthread_join(thrB, 0), pthread_join(thrL, 0), exit(0);
}