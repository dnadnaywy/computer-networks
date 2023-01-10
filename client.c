#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <ncurses.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

#define LITERE 1000
/* portul de conectare la server*/
int port;
/*declaratii de antet functii*/
int sendToServer(int sd, int nr);
char* readFromServer(int sd);
int CENTRALISTA(char mesaj[100],int sd);

/*declaratii de antet functii*/

bool INCHIDE = 0;

int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  // mesajul trimis
  int nr=0,statie=0;
  char buf[10];
  char mesaj[100],meniu[LITERE];

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

  
  if (read (sd, &meniu,LITERE) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
  {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
  }

  while (INCHIDE == 0)
  {
    nr=0;
    printf("%s",meniu);
    /* citirea mesajului */
    printf ("Introduceti numarul alegerii dvs: ");
    fflush (stdout);
    read (0, buf, sizeof(buf));
    nr=atoi(buf);

    while (nr>7 || nr < 1)
    {
      printf ("Nu exista nr indicat de dvs in meniu. Reintroduceti numarul alegerii dvs: ");
      fflush (stdout);
      read (0, buf, sizeof(buf));
      nr=atoi(buf);
    }

    /* trimiterea mesajului la server */
    if (write (sd,&nr,sizeof(int)) <= 0)
      {
        perror ("[client]Eroare la write() spre server.\n");
        return errno;
      }

    if (read (sd, &mesaj,100) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    //printf("mesaj este: %s\n",mesaj);
    CENTRALISTA(mesaj,sd);  /*fucntie pt diferite chestii cerute de server in plus, pt cazurile 1,2,3,4,5,6, deci toate ar trebui sa fie aici*/
    //printf("Am ajuns la finalul centralistei.\n");
    fflush(stdout);
  }
  /* inchidem conexiunea, am terminat */
  close (sd);
}

int sendToServer(int sd, int nr)
{
  if (write (sd,&nr,sizeof(int)) <= 0)
    {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }
}

char* readFromServer(int sd)
{
  char mesaj[100];
  if (read (sd, &mesaj,100) < 0)
    {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
    }
  return mesaj;
}

int CENTRALISTA(char mesaj[100],int sd)
{
  int statieSosire,statiePlecare, statie, id;
  char buffer[LITERE]="", temp[LITERE] = "";
  //printf("mesaj de la server: %s\n",mesaj);
  //printf("AM AJUNS LA CENTRALISTA\n");
  fflush(stdout);
  if (strcmp(mesaj,"statia")==0)
  {
    //printf("AM AJUNS IN CENTRALISTA LA O STATIE\n");
    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
    strcpy(mesaj,"Introduceti nr statiei:");
    printf ("\n[client]%s ", mesaj);
    scanf("%s",temp); //un buffer pt ce citeste de la client
    statie=atoi(temp);

    while (statie > 37 || statie < 1) /*verif daca e bine ce a introdus clientul de la tastatura*/
    {
      printf("Ati introdus un nr gresit. Reincercati acum: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      statie=atoi(temp);
    }

    sendToServer(sd,statie);

    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server cu informatiile cerute*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);

  }
  else if (strcmp(mesaj,"doua statii")==0)
  {
    //printf("AM AJUNS IN CENTRALISTA LA DOUA STATII\n");
    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
    strcpy(mesaj,"Introduceti numarul statiei de plecare: ");
    printf ("\n[client]%s ", mesaj);
    
    scanf("%s",temp); //un buffer pt ce citeste de la client
    statiePlecare=atoi(temp);

    while (statiePlecare > 37 || statiePlecare < 1) /*verif daca e bine ce a introdus clientul de la tastatura*/
    {
      printf("Ati introdus un nr gresit pentru statia de plecare. Reincercati acum: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      statiePlecare=atoi(temp);
    }

    strcpy(mesaj,"Introduceti numarul statiei de sosire: ");
    printf ("\n[client]%s ", mesaj);
    scanf("%s",temp); //un buffer pt ce citeste de la client
    statieSosire=atoi(temp);

    while (statieSosire > 37 || statieSosire < 1 || statieSosire == statiePlecare) /*verif daca e bine ce a introdus clientul de la tastatura*/
    {
      printf("Ori ati introdus un nr gresit pentru statia de sosire, ori acelasi numar ca la cea de plecare. Reincercati acum: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      statieSosire=atoi(temp);
    }

    sendToServer(sd,statiePlecare);
    sendToServer(sd,statieSosire);

    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server cu informatiile cerute*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
  }
  else if (strcmp(mesaj,"plecari")==0)
  {
    //printf("AM AJUNS IN CENTRALISTA LA PLECARI\n");
    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
    strcpy(mesaj,"Introduceti nr statiei:");
    printf ("\n[client]%s ", mesaj);

    /*verif daca e bine ce a introdus clientul de la tastatura*/
    scanf("%s",temp); //un buffer pt ce citeste de la client
    statie=atoi(temp);

    while (statie > 37 || statie < 1)
    {
      printf("Ati introdus un nr gresit. Reincercati acum: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      statie=atoi(temp);
    }

    sendToServer(sd,statie);

    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server cu informatiile cerute*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
  }
  else if (strcmp(mesaj,"sosiri")==0)
  {
    //printf("AM AJUNS IN CENTRALISTA LA SOSIRI\n");
    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
    strcpy(mesaj,"Introduceti nr statiei:");
    printf ("\n[client]%s ", mesaj);

     /*verif daca e bine ce a introdus clientul de la tastatura*/
    scanf("%s",temp); //un buffer pt ce citeste de la client
    statie=atoi(temp);

    while (statie > 37 || statie < 1)
    {
      printf("Ati introdus un nr gresit. Reincercati acum: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      statie=atoi(temp);
    }

    sendToServer(sd,statie);

    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server cu informatiile cerute*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
  }
  else if (strcmp(mesaj,"infoClient")==0)
  {
    int alegere, minute = 0;
    //printf("AM AJUNS IN CENTRALISTA LA INFOCLIENT\n");
    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
    strcpy(mesaj,"Introduceti alegerea dvs:");
    printf ("\n[client]%s ", mesaj);

    scanf("%s",temp); //un buffer pt ce citeste de la client
    alegere=atoi(temp);

    while (alegere > 3 || alegere < 1) //verif daca e bine ce a introdus clientul de la tastatura
    {
      printf("Ati introdus un nr gresit. Reincercati acum: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      alegere=atoi(temp);
    }

    sendToServer(sd,alegere);

    // acum ii trimit la server ce id are trenul pe care vreau sa il gasesc

    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
    
    strcpy(mesaj,"Introduceti ID-ul trenului, doar cele 4 cifre aflate dupa tipul trenului:");
    printf ("\n[client]%s ", mesaj);

    scanf("%s",temp); //un buffer pt ce citeste de la client
    id=atoi(temp);

    while (id != 1663 && id != 1977 && id != 1884 && id != 1153 && id != 1909) /*verif daca e bine ce a introdus clientul de la tastatura*/
    {
      printf("Ati introdus un id gresit. Reincercati acum: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      id=atoi(temp);
    }

    sendToServer(sd,id);

    if (alegere != 2)
    {
      printf("Introduceti minutele: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      minute=atoi(temp); //citeste, modifica, returneaza numere

      sendToServer(sd, minute);

    }
    if (read (sd, &buffer,LITERE) < 0) //aici se citeste mesajul de la server cu informatiile cerute
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s",buffer);
  }
  else if (strcmp(mesaj,"bradpitt") == 0) /*asta e pt estimare sosire*/
  {
    //printf("!!!estimareSosire\n");
    if (read (sd, &buffer,LITERE) < 0) /*aici se citeste mesajul de la server pt a introduce alte date specifice*/
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    //strcat(buffer,"Alegeti id-ul trenului dintre urmatoarele: 1. IR1663 - Bucuresti Nord->Iasi.\n2. RR1977 - Klaipeda->Vaslui.\n3.IR1884 - Constanta->BucurestiNord.\n4.IR1153 - Bucuresti Nord->Brasov.\n5.RR1909 - Chircesti->Klaipeda.\n");
    printf("\n%s",buffer);
    strcpy(mesaj,"Introduceti nr statiei:");
    printf ("\n[client]%s ", mesaj);

    scanf("%s",temp); //un buffer pt ce citeste de la client
    statie=atoi(temp);

    while (statie > 37 || statie < 1) /*verif daca e bine ce a introdus clientul de la tastatura*/
    {
      printf("Ati introdus un nr gresit. Reincercati acum: ");
      scanf("%s",temp); //un buffer pt ce citeste de la client
      statie=atoi(temp);
    }

    strcpy(mesaj,"Introduceti ID-ul trenului, doar cele 4 cifre aflate dupa tipul trenului:");
    printf ("\n[client]%s ", mesaj);
    
    scanf("%s",temp); //un buffer pt ce citeste de la client
    id=atoi(temp);

    while (id != 1663 && id != 1977 && id != 1884 && id != 1153 && id != 1909) /*verif daca e bine ce a introdus clientul de la tastatura*/
    {
      printf("Ati introdus un id gresit. Reincercati acum: ");
      
      scanf("%s",temp); //un buffer pt ce citeste de la client
      id=atoi(temp);
    }

    sendToServer(sd,statie);
    sendToServer(sd,id);

    if (read (sd, &buffer,LITERE) < 0) //aici se citeste mesajul de la server cu informatiile cerute
    {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }

    if (strcmp(buffer,"404ERROR")==0)
    {
      printf("Trenul cautat nu trece prin statia introdusa de dvs.\n");
    }
    else printf("\n%s",buffer);
  }
  else if (strcmp(mesaj, "exit") == 0)
  {
    INCHIDE = 1;
    printf("Va multumim! O zi frumoasa!\n");
  }
  else if (strcmp(mesaj,"eroare")==0)
  {
    strcpy(mesaj,"Comanda negasita, incercati altceva.\n");
    printf ("[client]%s ", mesaj);
  }
}