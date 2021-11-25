#include        <sys/types.h>  
#include        <sys/socket.h>  
#include        <sys/time.h>    
#include        <time.h>         
#include        <netinet/in.h>  
#include        <arpa/inet.h>  
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>

#define  SERVER_PORT   5001   /* 着信側の待ち受けポート番号 */
#define  MAXPKTLEN     1004   /* 最大パケット長(バイト)     */

int GetFileSize(FILE *fp);

int
main(int argc, char **argv)
{
  int    sockfd;
	struct sockaddr_in	servaddr;
	struct timeval tv;      /* 受信タイムアウトの待ち時間を設定する変数 */

	FILE   *fp;              /* 読み込むファイルの識別子   */
  long   filesize;         /* ファイルサイズ             */

  int  num_bytes_data = 0; /* 相手に送信するデータの長さ */
  int  send_seq_num  = 0;  /* パケットの順序番号         */
  int  num_bytes_pkt = 0;	 /* 相手に送信するパケットの長さ */

	char sendpkt[MAXPKTLEN];    /* パケット全体を格納する配列変数 */
	char senddata[MAXPKTLEN-4]; /* パケットのデータ部を格納する配列変数 */
        
  char recvpkt[MAXPKTLEN]; /* 相手から受信するACKパケット全体を */
  /* 格納する配列変数 */
	int  num_bytes_recv;     /* 相手から受信するACKパケットの長さ情報を */
  /* 格納する変数 */
  printf("%d\n", argc);
	if (argc == 1) {
    printf("usage: <IPaddress> <FileName> ...\n");
    return -1;
	}

	/* コマンド実行時に引数として与えるIPアドレスを相手のIPアドレス */
  /* として設定 */

	memset(&servaddr, 0, sizeof(servaddr));
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERVER_PORT);

  /* UDPのソケットを設定 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0) {
    perror("Socket open error:\n");
    return -1;
  }

  for (int i = 2; i < argc; i++) {
    /* 相手に送信する画像ファイルをオープン */
    if (NULL == (fp = fopen(argv[i],"rb"))) {
      perror("File open error:");
      return -1;
    }
    /* 受信タイムアウト時間の設定 */

    tv.tv_sec  = 0;      /* 秒単位 */
    tv.tv_usec = 750000; /* マイクロ秒単位 ここでは750us*/
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* ファイルサイズ (バイト数) を取得 */
    filesize = GetFileSize(fp);
    printf("File size is %ld bytes\n",filesize);

    /* ファイルサイズが1000バイト以上なら，1000バイトを，そうでなければ */
    /* ファイルサイズに等しい長さのデータを相手に送ると設定 */
    /* この数を変数 num_bytes_data に格納する */


    int j = 0;
    while(1) {
      if (filesize >= 1000) {
        num_bytes_data = 1000;
        filesize -= 1000;
        j++;
      } else {
        num_bytes_data = filesize;
        if (i == argc - 1) {
          j = 0;
        } else {
          j = -1;
        }
      }

      /* パケットの順序番号を 1と設定 */
      send_seq_num = j;

      /* num_bytes_dataの長さのデータをファイルから読み込んで */
      /* senddataに格納 */
      fread(senddata, num_bytes_data,1,fp);

      /* sendpktの先頭4バイトに順序番号の変数 send_seq_num(4バイト)の内容をコピー */
      memcpy(sendpkt,(char *)&send_seq_num,4);

      /* sendpktの先頭から5バイト以降に，senddataの内容を num_bytes_dataの長さだけコピー */
      memcpy(sendpkt+4,senddata,num_bytes_data);

      /* パケットの長さを num_bytes_pktに設定 */
      num_bytes_pkt = num_bytes_data + 4;

      while(1) {
        /* sendataの先頭から num_bytes_pkt　バイトを相手に送信 */
        sendto(sockfd, sendpkt, num_bytes_pkt, 0, 
              (struct sockaddr *)&servaddr, 
              sizeof(servaddr));

        printf("Sending data of %d bytes, seq_num=%d....",\
            num_bytes_data,send_seq_num);

        /* 相手から ACK パケットを受信 */
        num_bytes_recv
              = recvfrom(sockfd, recvpkt,MAXPKTLEN,0,NULL,NULL);

        if(num_bytes_recv >= 0) {
          break;
        } else {
          printf("ACK timeout.\n");
        }
      }

      if (num_bytes_recv>0) {
        printf("ACK received.\n");
      } else {
        /*num_bytes_recv=0の場合はタイムアウト以外のエラー*/
        printf("ACK error.\n");
      }

      if (j <= 0) {
        break;
      }
    }
  }

  fclose(fp);
  close(sockfd);
  return 0;
}


int GetFileSize(FILE *fp)    /* 引数で与えるファイルのサイズを出力する関数 */
{
  int filesize;

  fseek(fp,0L,SEEK_END); 
  filesize = ftell(fp);
  rewind(fp);

  return filesize;
}

