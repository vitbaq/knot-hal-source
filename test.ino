#include <abstract_driver.h>
#include <printf_serial.h>

// application packet size maximum
#define PACKET_SIZE_MAX    128

#define MESSAGE "the quick brown fox jumps over the lazy dog"
#define MESSAGE_SIZE  (sizeof(MESSAGE)-1)

int sock;
int channel = 10,
    nbytes,
    size,
    msg_count,
    count,
    inc;
bool connected = false;
char  msg[PACKET_SIZE_MAX],
      buffer[PACKET_SIZE_MAX+1];

void setup() {
  printf_serial_init();
  fprintf(stdout, "Client started...\n");

  if (nrf24l01_driver.probe(PACKET_SIZE_MAX) < 0) {
    fprintf(stderr, " probe error(%d)\n", errno);
    return;
  }

  fprintf(stdout, "NRF24L01 loaded\n");

  for (size=0, inc=MESSAGE_SIZE; size<sizeof(msg);) {
    if ((inc+size) > sizeof(msg)) {
      inc = sizeof(msg) - size;
    }
    memcpy(msg+size, MESSAGE, inc);
    size += inc;
  }
}

void loop() {
  if (!connected) {
    sock = nrf24l01_driver.socket();
    if (sock < 0) {
      fprintf(stderr, " socket error(%d)\n", errno);
      delay(1000);
      return;
    }
    
    fprintf(stdout, "Client connecting...\n");
    if (nrf24l01_driver.connect(sock, &channel, sizeof(channel)) < 0) {
      fprintf(stderr, " connect error(%d)\n", errno);
      nrf24l01_driver.close(sock);
      return;
    }
    
    fprintf(stdout, " connected ok\n");
    connected = true;
    count = 1;
    msg_count = 0;
  }
  memcpy(buffer, msg, count);
  nbytes = count;
  inc = (count == size) ? -1 : (count == 1 ? 1 : inc);
  count += inc;
  if (nrf24l01_driver.write(sock, buffer, nbytes) < 0) {
    fprintf(stderr, " write error(%d)\n", errno);
    nrf24l01_driver.close(sock);
    connected = false;
  } else {
    buffer[nbytes] = '\0';
    fprintf(stdout, "TX:[%03d]: %d-'%s'\n", nbytes, ++msg_count, buffer);
  }
  while(connected) {
    nbytes = nrf24l01_driver.read(sock, buffer, sizeof(buffer));
    if (nbytes < 0) {
      fprintf(stderr, " read error(%d)\n", errno);
      nrf24l01_driver.close(sock);
      connected = false;
    } else if (nbytes == 0) {
      delay(10);
    } else {
      buffer[nbytes] = '\0';
      fprintf(stdout, "RX:[%03d]: '%s'\n", nbytes, buffer);
      break;
    }
  }
}
