#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <ncurses.h>
#include <time.h>

/* portul folosit */
#define PORT 2908

/* definitii pentru alegerile posibile ale clientului */

#define LITERE 1000
#define MERS_TRENURI_O_STATIE 1
#define MERS_TRENURI_DOUA_STATII 2
#define PLECARI 3
#define SOSIRI 4
#define infoClient 5
#define estimareaSosirii 6
#define EXIT 7
#define ORASE "1. Chircesti       2. Ploiesti Sud     3. Vaslui         4. Mizil\n\
5. Buzau           6. Ramnicu Sarat    7. Focsani        8.Tecuci\n\
9. Barlad         10. Crasna          11. Buhaiesti     12. Barnova\n\
13. Nicolina      14. Iasi            15. Campina       16. Sinaia\n\
17. Busteni       18. Azuga           19. Predeal       20. Brasov\n\
21. Ploiesti Vest 22. Budapesta       23. Bratislava    24. Cracovia\n\
25. Varsovia      26. Kaliningrad     27. Klaipeda      28. Bucuresti Nord\n\
29. Constanta     30. Medgidia        31. Cernavoda Pod 32. Fetesti\n\
33. Ciulnita      34. Lehliu          35. Fundulea      36. Frasinu\n"

#define idTren "1. IR1663 - Bucuresti Nord->Iasi.\n2. RR1977 - Klaipeda->Vaslui.\n3. IR1884 - Constanta->BucurestiNord.\n4. IR1153 - Bucuresti Nord->Brasov.\n5. RR1909 - Chircesti->Klaipeda.\n"

/* codul de eroare returnat de anumite apeluri */
extern int errno;


typedef struct thData
{
  int idThread; // id-ul thread-ului tinut in evidenta de acest program
  int cl;       // descriptorul intors de accept
} thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
int raspunde(void *);
int alegeri_client(void *arg);
void mers_trenuri_o_statie(void *arg);
void fereastraGrafica();
void searchXmlOneStation(int statie, char mesaj[LITERE]);
void searchXmlTwoStations(int statiePlecare, int statieSosire, char mesaj[LITERE]);
int readFromClient(void *arg);
void sendToClient(char mesaj[100], void *arg);
void trainTwoStations(void *arg);
char *findCity(int nr); /*aici nr este numarul orasului, care se gasesc in ordine in orase.txt*/
void searchXmlPlecari(int statie, char mesaj[LITERE]);
void searchXmlSosiri(int statie, char mesaj[LITERE]);
void plecariDinStatie(void *arg);
void sosiriInStatie(void *arg);
void estimareSosire(void *arg);
void searchXmlEstimareSosiri(int statie, int ID, char mesaj[LITERE]);
int solveDelayArrivalTime(char arrivalTime[10],char delayArrival[10], char earlyArrival[10]);
void changeITOA(char val[10], int nr);
void functieInfoClient(void *arg);
void searchXmlInfoClient(int idClient, int alegere, char mesaj[LITERE], void *arg);
int timpLocal();

int main()
{
  struct sockaddr_in server; // structura folosita de server
  struct sockaddr_in from;
  int nr; // mesajul primit de trimis la client
  int sd; // descriptorul de socket
  int pid;
  pthread_t th[100]; // Identificatorii thread-urilor care se vor crea
  int i = 0;

  /* crearea unui socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[server]Eroare la socket().\n");
    return errno;
  }
  /* utilizarea optiunii SO_REUSEADDR */
  int on = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  /* pregatirea structurilor de date */
  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  /* utilizam un port utilizator */
  server.sin_port = htons(PORT);

  /* atasam socketul */
  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[server]Eroare la bind().\n");
    return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen(sd, 2) == -1)
  {
    perror("[server]Eroare la listen().\n");
    return errno;
  }
  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
  {
    int client;
    thData *td; // parametru functia executata de thread
    int length = sizeof(from);

    printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);

    // client= malloc(sizeof(int));
    /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
    if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
    {
      perror("[server]Eroare la accept().\n");
      continue;
    }

    /* s-a realizat conexiunea, se astepta mesajul */

    // int idThread; //id-ul threadului
    // int cl; //descriptorul intors de accept

    td = (struct thData *)malloc(sizeof(struct thData));
    td->idThread = i++;
    td->cl = client;

    pthread_create(&th[i], NULL, &treat, td);
    //printf("finish\n");

  } // while
};

static void *treat(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  alegeri_client(arg);  sleep(1);
  int INCHIDE = 0;
  while (INCHIDE == 0)
  {
    printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    INCHIDE = raspunde((struct thData *)arg);
  }
  /* am terminat cu acest client, inchidem conexiunea */
  close((intptr_t)arg);
  pthread_exit(NULL);
  return (NULL);
};

int raspunde(void *arg)
{
  int nr, i = 0;
  char mesaj[100];
  struct thData tdL;
  tdL = *((struct thData *)arg);
  /*if (read(tdL.cl, &nr, sizeof(int)) <= 0)
  {
    printf("[Thread %d]\n", tdL.idThread);
    perror("Eroare la read() de la client.\n");
  }*/
  nr = readFromClient(arg);
  printf("Prima alegere a clientului este: %d\n", nr);

  /*raspunsul serverului*/
  if (nr == MERS_TRENURI_O_STATIE)
  {
    strcpy(mesaj, "o statie");
    mers_trenuri_o_statie(arg);
  }
  else if (nr == MERS_TRENURI_DOUA_STATII)
  {
    strcpy(mesaj, "jennifer");
    trainTwoStations(arg);
  }
  else if (nr == PLECARI)
  {
    strcpy(mesaj, "plecari");
    plecariDinStatie(arg);
  }
  else if (nr == SOSIRI)
  {
    strcpy(mesaj, "sosiri");
    sosiriInStatie(arg);
  }
  else if (nr == infoClient)
  {
    strcpy(mesaj, "infoClient");
    functieInfoClient(arg);
  }
  else if (nr == estimareaSosirii)
  {
    strcpy(mesaj, "bradpitt");
    //printf("mesajul pt centralista este: %s\n",mesaj);
    estimareSosire(arg);
  }
  else if (nr == EXIT)
  {
    strcpy(mesaj,"exit");
    if (write(tdL.cl, &mesaj, 100) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
  }
  else
  {
    strcpy(mesaj, "eroare");
  }
  return 0;
  /*if (write(tdL.cl, &mesaj, 100) <= 0)
  {
    printf("[Thread %d] ", tdL.idThread);
    perror("[Thread]Eroare la write() catre client.\n");
  }
  else
    printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);*/
}

int alegeri_client(void *arg)
{
  char buffer[LITERE];
  struct thData tdL;
  tdL = *((struct thData *)arg);
  strcpy(buffer, "\n");
  strcat(buffer, "Introduceti cifra corespunzatoare selectiei dvs:\n");
  strcat(buffer, "1. Mersul trenurilor pentru ziua respectiva corespunzator unei statii.\n");
  strcat(buffer, "2. Mersul trenurilor pentru ziua respectiva intre doua statii.\n");
  strcat(buffer, "3. Plecari in urmatoarea ora.\n");
  strcat(buffer, "4. Sosiri in urmatoarea ora.\n");
  strcat(buffer, "5. Trimitere de info despre estimare sosire (orice abatere de la mersul obisnuit al trenurilor).\n");
  strcat(buffer, "6. Estimare sosire tren (cu informatii de la server).\n");
  strcat(buffer, "7. Exit.\n\n");
  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
}

void mers_trenuri_o_statie(void *arg)
{
  int nr, i = 0, statie = 0;
  char mesaj[100], buffer[LITERE] = "";
  struct thData tdL;
  tdL = *((struct thData *)arg);
  strcpy(buffer, "Scrieti nr corespunzator statiei din urmatoarele:\n");
  strcat(buffer, ORASE);
  strcpy(mesaj, "statia"); /*mesajul pentru centralista*/
  /*trimite catre client mesajul specific cazului 1*/
  if (send(tdL.cl,&mesaj,sizeof(mesaj),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
  // sendToClient(mesaj,(struct thData *)arg);
  /*trimite catre client mesajul specific cazului 1*/

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

  /*primeste rapsunsul cu nr statiei de la client*/
  statie = readFromClient(arg);
  printf("Statia citita este %s\n", findCity(statie));
  /*primeste rapsunsul cu nr statiei de la client*/

  // acum parcurgem xml si cautam orasul cu id = statie
  searchXmlOneStation(statie, buffer);

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

  // printf("[line 249]AM IESIT DIN XML\n");
}

void trainTwoStations(void *arg)
{
  int nr, i = 0, statiePlecare = 0, statieSosire = 0;
  char mesaj[100];
  char buffer[LITERE] = "";
  struct thData tdL;
  tdL = *((struct thData *)arg);
  strcpy(buffer, "Scrieti numerele corespunzatoare statiilor din urmatoarele:\n");
  strcat(buffer, ORASE);
  strcpy(mesaj, "doua statii"); /*mesajul pentru centralista*/
  /*trimite catre client mesajul specific cazului 1*/
  if (send(tdL.cl,&mesaj,sizeof(mesaj),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
  // sendToClient(mesaj,(struct thData *)arg);
  /*trimite catre client mesajul specific cazului 1*/
  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

  /*primeste rapsunsul cu nr statiei de la client*/
  statiePlecare = readFromClient((struct thData *)arg);
  statieSosire = readFromClient((struct thData *)arg);
  printf("Statia de plecare este %s", findCity(statiePlecare));
  printf("Statia de sosire este %s", findCity(statieSosire));
  /*primeste rapsunsul cu nr statiei de la client*/

  // acum parcurgem xml si cautam orasul cu id = statie
  strcpy(buffer,"400ERROR");
  searchXmlTwoStations(statiePlecare,statieSosire,buffer);

  if (strcmp(buffer,"400ERROR")==0)
  {
    strcpy(buffer,"Nu s-a gasit conexiune intre cele 2 statii date de dvs.\n");
  }

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

}

void searchXmlOneStation(int statie, char mesaj[LITERE])
{
  /*initializez mesajul de trimis catre client cu ""*/
  strcpy(mesaj, "");
  FILE *fd;
  char type[10] = "", id[10] = "";
  fd = fopen("trenuri.xml", "r");

  char *buffer = NULL, *copie = NULL;
  size_t len_buffer = 0;
  bool ok = 0, station = 0;                                       /*ok e pt cand am gasit un type si id; station e pt cand am gasit un tag <station>*/
  bool okStationName = 0, okArrivalTime = 0, okDepartureTime = 0; /*toate astea sunt pt a afisa toate info corecte cand compar verifStatie cu stationName*/
  int j, ct = 0;
  char stationName[100] = "", arrivalTime[10] = "", departureTime[10] = "";
  char verifStatie[100] = ""; /*din statie, care e nr, gasim numele luat din orase.txt si apoi salvam in verifStatie*/
  copie = findCity(statie);
  strncpy(verifStatie, copie, strlen(copie) - 1);
  // printf("verifStatie: %s; strlen(verifStatie): %d;\n",verifStatie,strlen(verifStatie));

  getline(&buffer, &len_buffer, fd); // parsing the xml until discovering the category
  while (strcmp(buffer, "</trainList>") != 0)
  {
    // parsing the xml until discovering the category
    getline(&buffer, &len_buffer, fd);
    if (strstr(buffer, "<type>") != NULL)
    {
      strncpy(type, buffer + 8, 2);
      getline(&buffer, &len_buffer, fd);
      strncpy(id, buffer + 6, 4);
      getline(&buffer, &len_buffer, fd);
      ok = 1;
    }
    if (ok == 1)
    {
      // printf("AM AJUNS INTR-UN TRAIN\n"); /*am ajuns intr-un train*/
      strcpy(stationName, "");
      strcpy(arrivalTime, "");
      strcpy(departureTime, "");
      okStationName = 0;
      okArrivalTime = 0;
      okDepartureTime = 0;
      while (strstr(buffer, "</train>") == NULL)
      {
        if (strstr(buffer, "<station>") != 0)
        {
          station = 1; /*AM AJUNS INTR-O STATION*/
        }
        if (station == 1)
        {
          station = 0;
          while (strstr(buffer, "</station>") == NULL) /*nu ajung la finalul unui station*/
          {
            if (strstr(buffer, "<stationName>") != 0)
            {
              /*deci gasesc stationName aici*/
              /*prelucrare stationName*/
              okArrivalTime = 0;
              okDepartureTime = 0;
              j = 0;
              char aux[100] = "";
              for (int i = 17; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(stationName, aux);
              okStationName = 1;
              // printf("Am gasit numele statiei: %s.\n",stationName);
            }
            if (strstr(buffer, "<arrivalTime>") != 0)
            {
              /*gasesc arrivalTime; prelucrarea arrivalTime*/
              okArrivalTime = 1;
              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu hour*/
              j = 0;
              char aux[100] = "";
              for (int i = 11; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(arrivalTime, aux);

              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu minutes*/
              j = 0;
              strcpy(aux, "");
              for (int i = 14; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcat(arrivalTime, ":");
              strcat(arrivalTime, aux); /*concatenez minutele la ore*/
              // printf("Am gasit arrivalTime si stationName: %s in %s.\n", arrivalTime, stationName);
            }
            if (strstr(buffer, "<departureTime>") != 0) /*gasesc departureTime; prelucrarea departureTime*/
            {
              okDepartureTime = 1;
              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu hour*/
              j = 0;
              char aux[100] = "";
              for (int i = 11; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(departureTime, aux);

              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu minutes*/
              j = 0;
              strcpy(aux, "");
              for (int i = 14; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcat(departureTime, ":");
              strcat(departureTime, aux); /*concatenez minutele la ore*/
              // printf("Am gasit departureTime si stationName: %s in %s.\n", departureTime, stationName);
            }
            if (strcmp(verifStatie, stationName) == 0 && okStationName == 1 && okArrivalTime == 1 && okDepartureTime == 1)
            {
              /*inseamna ca am gasit statia ceruta de client*/
              //printf("Trenul %s%s ajunge in %s la ora %s si pleaca la ora %s.\n", type, id, stationName, arrivalTime, departureTime);
              
              if (strcmp(departureTime,arrivalTime) == 0)
              {
                //avem cazul special
                strcpy(mesaj, "Trenul ");
                strcat(mesaj, type);
                strcat(mesaj, id);
                strcat(mesaj, " va fi in ");
                strcat(mesaj, stationName);
                strcat(mesaj, " la ora ");
                strcat(mesaj, arrivalTime);
                strcat(mesaj, ".\n");
                okStationName = 0;
                okArrivalTime = 0;
                okDepartureTime = 0;
              }
              else{
                strcpy(mesaj, "Trenul ");
                strcat(mesaj, type);
                strcat(mesaj, id);
                strcat(mesaj, " ajunge in ");
                strcat(mesaj, stationName);
                strcat(mesaj, " la ora ");
                strcat(mesaj, arrivalTime);
                strcat(mesaj, " si pleaca la ora ");
                strcat(mesaj, departureTime);
                strcat(mesaj, ".\n");
                okStationName = 0;
                okArrivalTime = 0;
                okDepartureTime = 0;
              }
            }
            getline(&buffer, &len_buffer, fd);
          }
        }
        getline(&buffer, &len_buffer, fd);
      }

      // printf("contorul este: %d\n",ct);
    }
    if (ok == 1)
    {
      // printf("type, id: %s, %s\n", type, id);
      ok = 0;
    }
  }
  // printf("[LINE 327]A IESIT DIN WHILE\n");
  fclose(fd);
}

void searchXmlTwoStations(int statiePlecare, int statieSosire, char mesaj[LITERE])
{
  /*initializez mesajul de trimis catre client cu ""*/
  //strcpy(mesaj, "");
  FILE *fd;
  char type[10] = "", id[10] = "";
  fd = fopen("trenuri.xml", "r");

  char *buffer = NULL, *copieSosire = NULL, *copiePlecare = NULL;
  size_t len_buffer = 0;
  bool ok = 0, gasitStationSosire = 0, gasitStationPlecare = 0, station = 0; /*ok e pt cand am gasit un type si id; station e pt cand am gasit un tag <station>*/
  bool okStationName = 0, okArrivalTime = 0, okDepartureTime = 0;            /*toate astea sunt pt a afisa toate info corecte cand compar verifStatie cu stationName*/
  int j, ct = 0;
  char VIA[LITERE] = ""; /*vector pt a pastra statiile intermediare intre statie de plecare si sosire*/
  char stationNamePlecare[100] = "", arrivalTimePlecare[10] = "", departureTimePlecare[10] = "";
  char stationNameSosire[100] = "", arrivalTimeSosire[10] = "", departureTimeSosire[10] = "";
  char stationName[100] = "", arrivalTime[10] = "", departureTime[10] = "";
  char verifStatieSosire[100] = "", verifStatiePlecare[100] = ""; /*din statie, care e nr, gasim numele luat din orase.txt si apoi salvam in verifStatie*/
  copieSosire = findCity(statieSosire);
  copiePlecare = findCity(statiePlecare);

  strncpy(verifStatieSosire, copieSosire, strlen(copieSosire) - 1);
  strncpy(verifStatiePlecare, copiePlecare, strlen(copiePlecare) - 1);
  //printf("verifStatie: %s; strlen(verifStatie): %d;\n",verifStatiePlecare,strlen(verifStatiePlecare));

  getline(&buffer, &len_buffer, fd); // parsing the xml until discovering the category
  while (strcmp(buffer, "</trainList>") != 0)
  {
    // parsing the xml until discovering the category
    getline(&buffer, &len_buffer, fd);
    if (strstr(buffer, "<type>") != NULL)
    {
      strncpy(type, buffer + 8, 2);
      getline(&buffer, &len_buffer, fd);
      strncpy(id, buffer + 6, 4);
      getline(&buffer, &len_buffer, fd);
      ok = 1;
    }
    if (ok == 1)
    {
      // printf("AM AJUNS INTR-UN TRAIN\n"); /*am ajuns intr-un train*/
      strcpy(stationName, "");
      strcpy(arrivalTime, "");
      strcpy(departureTime, "");
      okStationName = 0;
      okArrivalTime = 0;
      okDepartureTime = 0;
      gasitStationPlecare = 0;
      while (strstr(buffer, "</train>") == NULL)
      {
        if (strstr(buffer, "<station>") != 0)
        {
          station = 1; /*AM AJUNS INTR-O STATION*/
        }
        if (station == 1)
        {
          station = 0;
          while (strstr(buffer, "</station>") == NULL) /*nu ajung la finalul unui station*/
          {
            if (strstr(buffer, "<stationName>") != 0)
            {
              /*deci gasesc stationName aici*/
              /*prelucrare stationName*/
              okArrivalTime = 0;
              okDepartureTime = 0;
              j = 0;
              char aux[100] = "";
              for (int i = 17; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(stationName, aux);
              okStationName = 1;
              if (gasitStationPlecare == 1)
              {
                // deci avem statie de plecare, salvam tot ce e intermediar;
                strcat(VIA,stationName);
                strcat(VIA," - ");
              }
              // printf("Am gasit numele statiei: %s.\n",stationName);
            }
            if (strstr(buffer, "<arrivalTime>") != 0)
            {
              /*gasesc arrivalTime; prelucrarea arrivalTime*/
              okArrivalTime = 1;
              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu hour*/
              j = 0;
              char aux[100] = "";
              for (int i = 11; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(arrivalTime, aux);

              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu minutes*/
              j = 0;
              strcpy(aux, "");
              for (int i = 14; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcat(arrivalTime, ":");
              strcat(arrivalTime, aux); /*concatenez minutele la ore*/
              // printf("Am gasit arrivalTime si stationName: %s in %s.\n", arrivalTime, stationName);
            }
            if (strstr(buffer, "<departureTime>") != 0) /*gasesc departureTime; prelucrarea departureTime*/
            {
              okDepartureTime = 1;
              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu hour*/
              j = 0;
              char aux[100] = "";
              for (int i = 11; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(departureTime, aux);

              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu minutes*/
              j = 0;
              strcpy(aux, "");
              for (int i = 14; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcat(departureTime, ":");
              strcat(departureTime, aux); /*concatenez minutele la ore*/
              // printf("Am gasit departureTime si stationName: %s in %s.\n", departureTime, stationName);
            }

            /*Eu am timp de plecare, sosire si numele unei statii; Acum fac comparatii, potae gasesc statie de sosire sau statie de plecare*/

            if (strcmp(stationName, verifStatiePlecare) == 0 && okStationName == 1 && okArrivalTime == 1 && okDepartureTime == 1)
            {
              // am gasit statia de plecare cautata de client in acest tren
              // acum iau un bool cu gasitStatiePlecare
              // si mai fac un if dupa, in caz de gasesc statia de sosire si gasitStatiePlecare este 1
              gasitStationPlecare = 1;
              strcpy(departureTimePlecare,departureTime);
              strcpy(arrivalTimePlecare,arrivalTime);
              strcpy(stationNamePlecare,stationName);
              strcpy(VIA , stationName);
              strcat(VIA, " - ");
              okStationName = 0;
              okArrivalTime = 0;
              okDepartureTime = 0;
            }

            if (strcmp(stationName, verifStatieSosire) == 0 && gasitStationPlecare == 1 && okStationName == 1 && okArrivalTime == 1 && okDepartureTime == 1)
            {
              /*am gasit statia de sosire*/
              ct++;
              okStationName = 0;
              okArrivalTime = 0;
              okDepartureTime = 0;
              strcpy(departureTimeSosire,departureTime);
              strcpy(arrivalTimeSosire,arrivalTime);
              strcpy(stationNameSosire,stationName);
              //printf("Trenul %s%s pleaca la ora %s din %s si ajunge la ora %s in %s. Trece prin statiile %s\n",type,id,departureTimePlecare, stationNamePlecare, arrivalTimeSosire, stationNameSosire, VIA);
              strcpy(mesaj,"Trenul "); strcat(mesaj,type); strcat(mesaj,id); strcat(mesaj," pleaca la ora ");
              strcat(mesaj,departureTimePlecare); strcat(mesaj," din "); strcat(mesaj,stationNamePlecare); 
              strcat(mesaj," si ajunge la ora "); strcat(mesaj,arrivalTimeSosire); strcat(mesaj," in "); strcat(mesaj,stationNameSosire);
              strcat(mesaj,". Trece prin statiile "); strcat(mesaj,VIA); strcat(mesaj,".\n");
            }

            getline(&buffer, &len_buffer, fd);
          }
        }
        getline(&buffer, &len_buffer, fd);
      }

      // printf("contorul este: %d\n",ct);
    }
    if (ok == 1)
    {
      // printf("type, id: %s, %s\n", type, id);
      ok = 0;
    }
    //printf("Am gasit %d trenuri intre statiile precizate.\n",ct);
  }
  // printf("[LINE 327]A IESIT DIN WHILE\n");
  fclose(fd);
}

void plecariDinStatie(void *arg)
{
  int nr, i = 0, statie = 0;
  char mesaj[100], buffer[LITERE] = "";
  struct thData tdL;
  tdL = *((struct thData *)arg);
  strcpy(buffer, "Vreti sa aflati plecarile. Scrieti nr corespunzator statiei din urmatoarele:\n");
  strcat(buffer, ORASE);
  strcpy(mesaj, "plecari"); /*mesajul pentru centralista*/
  /*trimite catre client mesajul specific cazului 1*/
  if (send(tdL.cl,&mesaj,sizeof(mesaj),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
  // sendToClient(mesaj,(struct thData *)arg);
  /*trimite catre client mesajul specific cazului 1*/

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

  /*primeste rapsunsul cu nr statiei de la client*/
  statie = readFromClient(arg);
  printf("Statia citita este %s\n", findCity(statie));
  /*primeste rapsunsul cu nr statiei de la client*/

  // acum parcurgem xml si cautam orasul cu id = statie
  strcpy(buffer,"100ERROR");
  searchXmlPlecari(statie, buffer);
  
  if (strcmp(buffer,"100ERROR") == 0)
  {
    strcpy(buffer,"Nu sunt trenuri care sa plece in urmatoarea ora din statia indicata.\n");
  }

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
}

void searchXmlPlecari(int statie, char mesaj[LITERE])
{
  /*initializez mesajul de trimis catre client cu ""*/
  //strcpy(mesaj, "");
  FILE *fd;
  char type[10] = "", id[10] = "";
  fd = fopen("trenuri.xml", "r");

  int hour = timpLocal(), TIMP;
  printf("timpul actual este: %d\n",hour);
  char *buffer = NULL, *copie = NULL;
  size_t len_buffer = 0;
  bool ok = 0, station = 0;                    /*ok e pt cand am gasit un type si id; station e pt cand am gasit un tag <station>*/
  bool okStationName = 0, okDepartureTime = 0; /*toate astea sunt pt a afisa toate info corecte cand compar verifStatie cu stationName*/
  int j, ct = 0;
  int delayPlecare;
  char stationName[100] = "", departureTime[10] = "", delayDeparture[100] = "";
  char verifStatie[100] = ""; /*din statie, care e nr, gasim numele luat din orase.txt si apoi salvam in verifStatie*/
  copie = findCity(statie);
  strncpy(verifStatie, copie, strlen(copie) - 1);
  // printf("verifStatie: %s; strlen(verifStatie): %d;\n",verifStatie,strlen(verifStatie));

  getline(&buffer, &len_buffer, fd); // parsing the xml until discovering the category
  while (strcmp(buffer, "</trainList>") != 0)
  {
    // parsing the xml until discovering the category
    getline(&buffer, &len_buffer, fd);
    if (strstr(buffer, "<type>") != NULL)
    {
      strncpy(type, buffer + 8, 2);
      getline(&buffer, &len_buffer, fd);
      strncpy(id, buffer + 6, 4);
      getline(&buffer, &len_buffer, fd);
      ok = 1;
    }
    if (ok == 1)
    {
      // printf("AM AJUNS INTR-UN TRAIN\n"); /*am ajuns intr-un train*/
      strcpy(stationName, "");
      strcpy(departureTime, "");
      okStationName = 0;
      okDepartureTime = 0;
      while (strstr(buffer, "</train>") == NULL)
      {
        if (strstr(buffer, "<station>") != 0)
        {
          station = 1; /*AM AJUNS INTR-O STATION*/
        }
        if (station == 1)
        {
          station = 0;
          while (strstr(buffer, "</station>") == NULL) /*nu ajung la finalul unui station*/
          {
            if (strstr(buffer, "<stationName>") != 0)
            {
              /*deci gasesc stationName aici*/
              /*prelucrare stationName*/
              okDepartureTime = 0;
              j = 0;
              char aux[100] = "";
              for (int i = 17; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(stationName, aux);
              okStationName = 1;
              // printf("Am gasit numele statiei: %s.\n",stationName);
            }
            if (strstr(buffer, "<departureTime>") != 0) /*gasesc departureTime; prelucrarea departureTime*/
            {
              okDepartureTime = 1;
              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu hour*/
              j = 0;
              char aux[100] = "";
              for (int i = 11; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(departureTime, aux);
              TIMP = atoi(departureTime);

              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu minutes*/
              j = 0;
              strcpy(aux, "");
              for (int i = 14; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcat(departureTime, ":");
              strcat(departureTime, aux); /*concatenez minutele la ore*/
              // printf("Am gasit departureTime si stationName: %s in %s.\n", departureTime, stationName);

              getline(&buffer, &len_buffer, fd); /*suntem la linia cu delayDepartures*/
              getline(&buffer, &len_buffer, fd);
              j = 0;
              strcpy(aux, "");
              for (int i = 20; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[j] = '\0';
              strcpy(delayDeparture, aux);
              /*copiez timpul de delay la plecare din xml*/
              // printf("delay la plecare: %s\n", delayDeparture);
            }
            if (strcmp(verifStatie, stationName) == 0 && okStationName == 1 && okDepartureTime == 1 && TIMP == hour)
            {
              /*inseamna ca am gasit statia ceruta de client*/
              delayPlecare = atoi(delayDeparture);
              if (delayPlecare != 0)
              {
                printf("Trenul %s%s trebuia sa ajunga in %s si sa plece la ora %s. Are intarziere de %s minut(e).\n", type, id, stationName, departureTime, delayDeparture);
                strcpy(mesaj, "Trenul ");
                strcat(mesaj, type);
                strcat(mesaj, id);
                strcat(mesaj, " trebuia sa ajunga in ");
                strcat(mesaj, stationName);
                strcat(mesaj, " si sa plece la ora ");
                strcat(mesaj, departureTime);
                strcat(mesaj, ". Are intarziere de ");
                strcat(mesaj, delayDeparture);
                strcat(mesaj, " minut(e).\n");
              }
              else
              {
                printf("Trenul %s%s ajunge in %s si pleaca la ora %s. Nu are intarziere trenul in statie.\n", type, id, stationName, departureTime);
                strcpy(mesaj, "Trenul ");
                strcat(mesaj, type);
                strcat(mesaj, id);
                strcat(mesaj, " ajunge in ");
                strcat(mesaj, stationName);
                strcat(mesaj, " si pleaca la ora ");
                strcat(mesaj, departureTime);
                strcat(mesaj, ". Nu are intarziere trenul in statie.\n");
              }
              okStationName = 0;
              okDepartureTime = 0;
            }
            getline(&buffer, &len_buffer, fd);
          }
        }
        getline(&buffer, &len_buffer, fd);
      }
    }
    if (ok == 1)
    {
      // printf("type, id: %s, %s\n", type, id);
      ok = 0;
    }
  }
  fclose(fd);
}

void sosiriInStatie(void *arg)
{
  int nr, i = 0, statie = 0;
  char mesaj[100], buffer[LITERE] = "";
  struct thData tdL;
  tdL = *((struct thData *)arg);
  strcpy(buffer, "Vreti sa aflati sosirile. Scrieti nr corespunzator statiei din urmatoarele:\n");
  strcat(buffer, ORASE);
  strcpy(mesaj, "sosiri"); /*mesajul pentru centralista*/
  /*trimite catre client mesajul specific cazului 1*/
  if (send(tdL.cl,&mesaj,sizeof(mesaj),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
  // sendToClient(mesaj,(struct thData *)arg);
  /*trimite catre client mesajul specific cazului 1*/

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

  /*primeste rapsunsul cu nr statiei de la client*/
  statie = readFromClient(arg);
  printf("Statia citita este %s\n", findCity(statie));
  /*primeste rapsunsul cu nr statiei de la client*/

  // acum parcurgem xml si cautam orasul cu id = statie
  strcpy(buffer,"200ERROR");
  searchXmlSosiri(statie, buffer);

  if (strcmp(buffer,"200ERROR") == 0)
  {
    strcpy(buffer,"Nu s-au gasit trenuri care sa plece din statia indicata de dvs in urmatoarea ora.\n");
  }

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
}

void searchXmlSosiri(int statie, char mesaj[LITERE])
{
  /*initializez mesajul de trimis catre client cu ""*/
  //strcpy(mesaj, "");
  FILE *fd;
  char type[10] = "", id[10] = "";
  fd = fopen("trenuri.xml", "r");

  int hour = timpLocal(), TIMP;
  printf("timpul actual este: %d\n",hour);
  char *buffer = NULL, *copie = NULL;
  size_t len_buffer = 0;
  bool ok = 0, station = 0;                  /*ok e pt cand am gasit un type si id; station e pt cand am gasit un tag <station>*/
  bool okStationName = 0, okArrivalTime = 0; /*toate astea sunt pt a afisa toate info corecte cand compar verifStatie cu stationName*/
  int j, ct = 0;
  int delaySosire, earlySosire;
  char stationName[100] = "", arrivalTime[10] = "", delayArrival[10] = "", earlyArrival[10] = "";
  char verifStatie[100] = ""; /*din statie, care e nr, gasim numele luat din orase.txt si apoi salvam in verifStatie*/
  copie = findCity(statie);
  strncpy(verifStatie, copie, strlen(copie) - 1);
  // printf("verifStatie: %s; strlen(verifStatie): %d;\n",verifStatie,strlen(verifStatie));

  getline(&buffer, &len_buffer, fd); // parsing the xml until discovering the category
  while (strcmp(buffer, "</trainList>") != 0)
  {
    // parsing the xml until discovering the category
    getline(&buffer, &len_buffer, fd);
    if (strstr(buffer, "<type>") != NULL)
    {
      strncpy(type, buffer + 8, 2);
      getline(&buffer, &len_buffer, fd);
      strncpy(id, buffer + 6, 4);
      getline(&buffer, &len_buffer, fd);
      ok = 1;
    }
    if (ok == 1)
    {
      // printf("AM AJUNS INTR-UN TRAIN\n"); /*am ajuns intr-un train*/
      strcpy(stationName, "");
      strcpy(arrivalTime, "");
      okStationName = 0;
      okArrivalTime = 0;
      while (strstr(buffer, "</train>") == NULL)
      {
        if (strstr(buffer, "<station>") != 0)
        {
          station = 1; /*AM AJUNS INTR-O STATION*/
        }
        if (station == 1)
        {
          station = 0;
          while (strstr(buffer, "</station>") == NULL) /*nu ajung la finalul unui station*/
          {
            if (strstr(buffer, "<stationName>") != 0)
            {
              /*deci gasesc stationName aici*/
              /*prelucrare stationName*/
              okArrivalTime = 0;
              j = 0;
              char aux[100] = "";
              for (int i = 17; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(stationName, aux);
              okStationName = 1;
              // printf("Am gasit numele statiei: %s.\n",stationName);
            }
            if (strstr(buffer, "<arrivalTime>") != 0)
            {
              /*gasesc arrivalTime; prelucrarea arrivalTime*/
              okArrivalTime = 1;
              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu hour*/
              j = 0;
              char aux[100] = "";
              for (int i = 11; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(arrivalTime, aux);
              TIMP = atoi(arrivalTime);

              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu minutes*/
              j = 0;
              strcpy(aux, "");
              for (int i = 14; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcat(arrivalTime, ":");
              strcat(arrivalTime, aux); /*concatenez minutele la ore*/
              // printf("Am gasit arrivalTime si stationName: %s in %s.\n", arrivalTime, stationName);

              getline(&buffer, &len_buffer, fd); /*suntem la linia cu delayArrival*/
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              // printf("%s", buffer);
              j = 0;
              strcpy(aux, "");
              for (int i = 18; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[j] = '\0';
              strcpy(delayArrival, aux);

              getline(&buffer, &len_buffer, fd); /*suntem la linia cu earlyArrival*/
              j = 0;
              strcpy(aux, "");
              for (int i = 18; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[j] = '\0';
              strcpy(earlyArrival, aux);
            }
            if (strcmp(verifStatie, stationName) == 0 && okStationName == 1 && okArrivalTime == 1 && TIMP == hour)
            {
              /*inseamna ca am gasit statia ceruta de client*/
              //printf("TIMP este : %d\n", TIMP);

              delaySosire = atoi(delayArrival);
              earlySosire = atoi(earlyArrival);
              if (delaySosire == 0 && earlySosire == 0)
              {
                printf("Trenul %s%s ajunge in %s la ora %s. A ajuns conform cu planul.\n", type, id, stationName, arrivalTime);
                strcpy(mesaj, "Trenul ");
                strcat(mesaj, type);
                strcat(mesaj, id);
                strcat(mesaj, " ajunge in ");
                strcat(mesaj, stationName);
                strcat(mesaj, " la ora ");
                strcat(mesaj, arrivalTime);
                strcat(mesaj, ". A ajuns conform cu planul.\n");
              }
              else if (delaySosire != 0)
              {
                printf("Trenul %s%s trebuia sa ajunga in %s la ora %s, dar a avut intarziere de %d minut(e).\n", type, id, stationName, arrivalTime, delaySosire);
                strcpy(mesaj, "Trenul ");
                strcat(mesaj, type);
                strcat(mesaj, id);
                strcat(mesaj, " trebuia sa ajunga in ");
                strcat(mesaj, stationName);
                strcat(mesaj, " la ora ");
                strcat(mesaj, arrivalTime);
                strcat(mesaj, ", dar a avut intarziere de ");
                strcat(mesaj, delayArrival);
                strcat(mesaj, " minut(e).\n");
              }
              else if (earlySosire != 0)
              {
                printf("Trenul %s%s trebuia sa ajunga in %s la ora %s, dar a ajuns mai devreme cu %d minut(e).\n", type, id, stationName, arrivalTime, earlySosire);
                strcpy(mesaj, "Trenul ");
                strcat(mesaj, type);
                strcat(mesaj, id);
                strcat(mesaj, " trebuia sa ajunga in ");
                strcat(mesaj, stationName);
                strcat(mesaj, " la ora ");
                strcat(mesaj, arrivalTime);
                strcat(mesaj, ", dar a ajuns mai devreme cu ");
                strcat(mesaj, earlyArrival);
                strcat(mesaj, " minut(e).\n");
              }
              okStationName = 0;
              okArrivalTime = 0;
            }
            getline(&buffer, &len_buffer, fd);
          }
        }
        getline(&buffer, &len_buffer, fd);
      }

      // printf("contorul este: %d\n",ct);
    }
    if (ok == 1)
    {
      // printf("type, id: %s, %s\n", type, id);
      ok = 0;
    }
  }
  // printf("[LINE 327]A IESIT DIN WHILE\n");
  fclose(fd);
}

void estimareSosire(void *arg)
{
  int nr, i = 0, statie = 0, id =0;
  char mesaj[100], buffer[LITERE] = "";
  struct thData tdL;
  tdL = *((struct thData *)arg);
  strcpy(buffer, "Scrieti nr corespunzator statiei din urmatoarele:\n");
  strcat(buffer, ORASE);
  strcat(buffer,"\nAlegeti id-ul trenului dintre urmatoarele:\n");
  strcat(buffer,idTren);
  strcpy(mesaj, "bradpitt"); /*mesajul pentru centralista*/
  /*trimite catre client mesajul specific cazului 1*/
  if (send(tdL.cl,&mesaj,sizeof(mesaj),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
  // sendToClient(mesaj,(struct thData *)arg);
  /*trimite catre client mesajul specific cazului 1*/

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

  /*primeste rapsunsul cu nr statiei de la client*/
  statie = readFromClient(arg);
  id=readFromClient(arg);
  printf("Statia citita este %s\n", findCity(statie));
  printf("ID-ul citit este: %d\n",id);
  /*primeste rapsunsul cu nr statiei de la client*/

  // acum parcurgem xml si cautam orasul cu id = statie
  strcpy(buffer,"404ERROR");
  searchXmlEstimareSosiri(statie, id, buffer);
  //printf("buffer este: %s\n", buffer);
  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
}

void searchXmlEstimareSosiri(int statie, int ID, char mesaj[LITERE])
{
  /*initializez mesajul de trimis catre client cu ""*/
  //strcpy(mesaj, "");
  FILE *fd;
  char type[10] = "", id[10] = "";
  fd = fopen("trenuri.xml", "r");

  char *buffer = NULL, *copie = NULL;
  size_t len_buffer = 0;
  bool ok = 0, station = 0;                                       /*ok e pt cand am gasit un type si id; station e pt cand am gasit un tag <station>*/
  bool okStationName = 0, okArrivalTime = 0, okDepartureTime = 0; /*toate astea sunt pt a afisa toate info corecte cand compar verifStatie cu stationName*/
  int j, ct = 0, verifID;
  char stationName[100] = "", arrivalTime[10] = "";
  char delayArrival[10] = "", earlyArrival[10] = "", delayDeparture[10] = "";
  char verifStatie[100] = ""; /*din statie, care e nr, gasim numele luat din orase.txt si apoi salvam in verifStatie*/
  copie = findCity(statie);
  strncpy(verifStatie, copie, strlen(copie) - 1);
  //printf("verifStatie: %s; strlen(verifStatie): %d;\n",verifStatie,strlen(verifStatie));

  getline(&buffer, &len_buffer, fd); // parsing the xml until discovering the category
  while (strcmp(buffer, "</trainList>") != 0)
  {
    // parsing the xml until discovering the category
    getline(&buffer, &len_buffer, fd);
    if (strstr(buffer, "<type>") != NULL)
    {
      strncpy(type, buffer + 8, 2);
      //printf("type is %s\n",type);
      getline(&buffer, &len_buffer, fd);
      strncpy(id, buffer + 6, 4);
      getline(&buffer, &len_buffer, fd);
      verifID=atoi(id);
      //printf("verifID este: %d\n",verifID);
      ok = 1;
    }
    if (ok == 1)
    {
      //printf("AM AJUNS INTR-UN TRAIN\n"); /*am ajuns intr-un train*/
      strcpy(stationName, ""); strcpy(arrivalTime, ""); strcpy(delayArrival, "");//strcpy(departureTime, "");
      okStationName = 0; okArrivalTime = 0; okDepartureTime = 0;
      while (strstr(buffer, "</train>") == NULL)
      {
        if (strstr(buffer, "<station>") != 0)
        {
          station = 1; /*AM AJUNS INTR-O STATION*/
        }
        if (station == 1)
        {
          station = 0;
          while (strstr(buffer, "</station>") == NULL) /*nu ajung la finalul unui station*/
          {
            if (strstr(buffer, "<stationName>") != NULL)
            {
              /*deci gasesc stationName aici*/
              /*prelucrare stationName*/
              okArrivalTime = 0;
              okDepartureTime = 0;
              j = 0;
              char aux[100] = "";
              for (int i = 17; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(stationName, aux);
              okStationName = 1;
              //printf("Am gasit numele statiei: %s.\n",stationName);
            }
            if (strstr(buffer, "<arrivalTime>") != NULL)
            {
              /*gasesc arrivalTime; prelucrarea arrivalTime*/
              okArrivalTime = 1;
              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu hour*/
              j = 0;
              char aux[100] = "";
              for (int i = 11; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcpy(arrivalTime, aux);

              getline(&buffer, &len_buffer, fd); /*acum suntem la linia cu minutes*/
              j = 0;
              strcpy(aux, "");
              for (int i = 14; buffer[i] != '<'; i++, j++)
                aux[j] = buffer[i];
              aux[strlen(aux)] = '\0';
              strcat(arrivalTime, ":");
              strcat(arrivalTime, aux); /*concatenez minutele la ore*/
               //printf("Am gasit arrivalTime si stationName: %s in %s.\n", arrivalTime, stationName);
            
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd);
              getline(&buffer, &len_buffer, fd); /*am ajuns la linia cu delayArrival*/
              //printf("delay arrival: %s", buffer);
              j = 0;
              strcpy(aux, "");
              for (int i = 18; buffer[i] != '<'; i++, j++)
                {
                aux[j] = buffer[i];
                //printf("buffer[i]: %c\n",buffer[i]);
              }
              aux[j] = '\0';
              strcpy(delayArrival, aux);

              getline(&buffer, &len_buffer, fd); /*am ajuns la linia cu earlyArrival*/
              //printf("buffer: %s\n",buffer);
              j = 0;
              strcpy(aux, "");
              for (int i = 18; buffer[i] != '<'; i++, j++)
              {
                aux[j] = buffer[i];
                //printf("buffer[i]: %c\n",buffer[i]);
              }
              aux[j] = '\0';
              strcpy(earlyArrival, aux);
            }
            fflush(stdout);
            //printf("verif id %d si id %d\nverifstatie %s si stationname %s\n",verifID,ID,verifStatie,stationName);
            if ( verifID==ID && strcmp(verifStatie, stationName) == 0 && okStationName == 1 && okArrivalTime == 1)
            {
              /*inseamna ca am gasit statia ceruta de client*/
              //printf("arrivalTime,delayArrival,earlyArrival: %s, %s, %s\n",arrivalTime,delayArrival,earlyArrival);
              solveDelayArrivalTime(arrivalTime,delayArrival,earlyArrival);
              //printf("Trenul cu ID-ul %s va ajunge in %s la ora %s. Va stationa %s minut(e) si va pleca la ora %s.\n", ID, stationName, );
              strcpy(mesaj, "Trenul "); strcat(mesaj, type);
              strcat(mesaj, id);
              strcat(mesaj, " ajunge in ");
              strcat(mesaj, stationName);
              strcat(mesaj, " la ora ");
              strcat(mesaj, arrivalTime);
              int delay, early;
              delay=atoi(delayArrival); early=atoi(earlyArrival);
              if (delay==0 && early == 0)
              {
                strcat(mesaj," conform cu planul.\n");
              }
              else if (delay != 0)
              {
                strcat(mesaj, " cu intarziere de ");
                strcat(mesaj,delayArrival);
                strcat(mesaj," minut(e).\n");
              }
              else if (early != 0)
              {
                strcat(mesaj, " mai devreme cu ");
                strcat(mesaj,earlyArrival);
                strcat(mesaj," minut(e).\n");
              }
              okStationName = 0; okArrivalTime = 0;
            }
            getline(&buffer, &len_buffer, fd);
          }
        }
        getline(&buffer, &len_buffer, fd);
      }

      // printf("contorul este: %d\n",ct);
    }
    if (ok == 1)
    {
      // printf("type, id: %s, %s\n", type, id);
      ok = 0;
    }
  }
  // printf("[LINE 327]A IESIT DIN WHILE\n");
  //printf("mesaj este: %s\n", mesaj);
  fclose(fd);
}

int solveDelayArrivalTime(char arrivalTime[10],char delayArrival[10], char earlyArrival[10])
{
  /*de facut pe maine??*/
  int delay, early, oreDeAdaugat, minuteDeAdaugat, ora = 0, minut = 0;
  int oreDeScazut, minuteDeScazut = 0;
  char ore[10], minute[10];  
  //changeITOA(ore,12);
  //printf("arrTime este: %s\n", arrivalTime);
  strncpy(ore,arrivalTime,2); strcpy(minute,arrivalTime+3); //preiau din arrivalTime ora si minute
  ora=atoi(ore); minut=atoi(minute); //le duc in int
  //printf("ora: %d; minut: %d\n",ora,minut);
  delay=atoi(delayArrival); early=atoi(earlyArrival); //fac delay si ealy sa fie int uri
  //printf("delay: %d, early: %d\n",delay,early);
  if (delay != 0)
  {
    //avem timp de sosire, la care tre sa adaug timpul de delay;
    //problema este ca daca avem, de ex, 15:56 si delay de 10 minute, ora tre sa fie 16:06
    oreDeAdaugat=delay/60; ora+=oreDeAdaugat;
    minuteDeAdaugat=delay%60;
    //printf("oreDeAdaugat: %d, minuteDeAdaugat: %d\n",oreDeAdaugat,minuteDeAdaugat);
    minut+=minuteDeAdaugat; /*acum, minutele pot depasi cele 60 alocate. atunci fac oe cazuri*/
    //printf("minut: %d\n",minut);
    if (minut>=60)
    {
      minut=minut%60;
      ora++;
    }
    if (ora>23)
    {
      ora=ora%24;
    }
    //printf("minut: %d\n",minut);
    //printf("chiar inainte de itoa ora: %d; minut: %d\n",ora,minut);
    strcpy(ore,"");strcpy(minute,"");
    changeITOA(ore,ora);
    if (minut < 10)
    {
      minute[0]='0';
      minute[1]=minut+'0';
    }
    else changeITOA(minute,minut);
    if (ora < 10)
    {
      ore[0]='0';
      ore[1]=ora+'0';
    }
    else changeITOA(ore,ora);
    //printf("ora noua este: %s:%s\n",ore,minute);
    strcpy(arrivalTime,ore); strcat(arrivalTime,":"); strcat(arrivalTime,minute);
    //printf("ora buna de trimis catre client: %s\n",arrivalTime);
  }
  if (early != 0)
  {
    oreDeScazut=early/60; ora-=oreDeScazut;
    minuteDeScazut=early%60;
    minut-=minuteDeScazut; /*acum, minutele pot depasi cele 60 alocate. atunci fac oe cazuri*/
    //printf("oreDeScazut: %d, minuteDeScazut: %d\n",oreDeScazut,minuteDeScazut);
    //printf("minut: %d\n",minut);
    if (minut<0)
    {
      minut=minut+60;
      ora--;
    }
    if (ora<0)
    {
      ora=ora+24;
    }
    strcpy(ore,"");strcpy(minute,"");
    changeITOA(ore,ora);
    if (minut < 10)
    {
      minute[0]='0';
      minute[1]=minut+'0';
    }
    else changeITOA(minute,minut);
    strcpy(arrivalTime,ore); strcat(arrivalTime,":"); strcat(arrivalTime,minute);
    //printf("ora buna de trimis catre client: %s\n",arrivalTime);
  }
}

void functieInfoClient(void *arg)
{
  int nr, i = 0, statie = 0;
  char mesaj[100], buffer[LITERE] = "";
  struct thData tdL;
  tdL = *((struct thData *)arg);
  strcpy(buffer, "Ce doriti sa actualizati?\n1. Tren intarziat;\n2. Tren ajuns la timp;\n3. Tren ajuns mai devreme;\n");
  //strcat(buffer, ORASE);
  strcpy(mesaj, "infoClient"); /*mesajul pentru centralista*/
  /*trimite catre client mesajul specific cazului 1*/
  if (send(tdL.cl,&mesaj,sizeof(mesaj),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
  // sendToClient(mesaj,(struct thData *)arg);
  /*trimite catre client mesajul specific cazului 1*/

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

  /*primeste rapsunsul cu nr statiei de la client*/
  int alegere, id;
  alegere = readFromClient(arg);
  printf("Alegerea citita este %d\n", alegere);

  strcpy(buffer,"\nAlegeti id-ul trenului dintre urmatoarele:\n");
  strcat(buffer,idTren);

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

  id=readFromClient(arg);
  printf("id citit este %d\n", id);

  // acum parcurgem xml si cautam orasul cu id = statie

  searchXmlInfoClient(id, alegere, buffer, arg);

  if (send(tdL.cl,&buffer,sizeof(buffer),MSG_NOSIGNAL) <= 0)
    {
      printf("[Thread %d] ", tdL.idThread);
      perror("[Thread]Eroare la write() catre client.\n");
      return -1;
    }
    else
      printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);

}

void searchXmlInfoClient(int idClient, int alegere, char mesaj[LITERE], void *arg)
{
  strcpy(mesaj, "");
  FILE *fd, *tmp;
  char type[10] = "", id[10] = "", intarziere[10] = "";
  int minute;

  //printf("the hour is %d\n", localtime(time(NULL))->tm_hour);
  fd = fopen("trenuri.xml", "r");
  tmp = fopen("xml.tmp","w");

  char *buffer = NULL, *copie = NULL;
  size_t len_buffer = 0;
   //printf("AM AJUNS INAINTE DE AASJLF\n");
  if (alegere == 1)     //avem un tren intarziat, trebuie citita si intarziere
  {
    //mai bagam o citire, a intarzierii
    minute=readFromClient(arg); //acum avem si minutelw cu care intarzie

    bool ok = 0;     

    getline(&buffer, &len_buffer, fd); // parsing the xml until discovering the category
    //printf("[1]buffer is: %s\n",buffer);
    fprintf(tmp, "%s", buffer);
    while (strcmp(buffer, "</trainList>") != 0)
    {
      // parsing the xml until discovering the category
      //printf("am ajuns aixi\n");
      getline(&buffer, &len_buffer, fd);  fprintf(tmp, "%s", buffer);
      //printf("[2]buffer is: %s\n",buffer);
      if (strstr(buffer, "<type>") != NULL)
      {
        strncpy(type, buffer + 8, 2);
        getline(&buffer, &len_buffer, fd); fprintf(tmp, "%s", buffer);
        //printf("[3]buffer is: %s\n",buffer);
        strncpy(id, buffer + 6, 4);
        getline(&buffer, &len_buffer, fd); fprintf(tmp, "%s", buffer);
        //printf("[4]buffer is: %s\n",buffer);
        ok = 1;
      }
      if (ok == 1 && atoi(id)==idClient)
      {
        //printf("AM AJUNS IN EGALITATEA \n");
        while (strstr(buffer, "</train>") == NULL)// && strcmp(buffer, "</trainList>") != 0)
        {
          getline(&buffer, &len_buffer, fd); 
          //printf("[5]buffer is: %s\n",buffer);
          if (strstr(buffer,"<delayDeparture>") != NULL)
          {
            //atunci valoarea trebuie schimbata in 0
            strcpy(buffer,"				<delayDeparture>");
            changeITOA(intarziere,minute);
            //printf("intarziere este: %s\n", intarziere);
            //sleep(2);
            strcat(buffer,intarziere);
            strcat(buffer,"</delayDeparture>\n");
          }
          if (strstr(buffer,"<delayArrival>") != NULL)
          {
            //atunci valoarea trebuie schimbata in 0
            strcpy(buffer,"				<delayArrival>");
            changeITOA(intarziere,minute);
            //printf("intarziere este: %s\n", intarziere);
            sleep(2);
            strcat(buffer,intarziere);
            strcat(buffer,"</delayArrival>\n");
          }
          fprintf(tmp, "%s", buffer);
        }
      }
      ok=0;
    }
    fclose(fd);
    fclose(tmp);
    fd = fopen("trenuri.xml","w");
    tmp = fopen("xml.tmp","r");
    while(getline(&buffer, &len_buffer, tmp) != -1)
    {
      fprintf(fd,"%s",buffer);
    }
    fclose(fd);
    fclose(tmp);
  }
  else if (alegere == 2)     //inseamna ca si delay si early sunt toate 0
  {
    bool ok = 0;     

    getline(&buffer, &len_buffer, fd); // parsing the xml until discovering the category
    //printf("[1]buffer is: %s\n",buffer);
    fprintf(tmp, "%s", buffer);
    while (strcmp(buffer, "</trainList>") != 0)
    {
      // parsing the xml until discovering the category
      //printf("am ajuns aixi\n");
      getline(&buffer, &len_buffer, fd);  fprintf(tmp, "%s", buffer);
      //printf("[2]buffer is: %s\n",buffer);
      if (strstr(buffer, "<type>") != NULL)
      {
        strncpy(type, buffer + 8, 2);
        getline(&buffer, &len_buffer, fd); fprintf(tmp, "%s", buffer);
        //printf("[3]buffer is: %s\n",buffer);
        strncpy(id, buffer + 6, 4);
        getline(&buffer, &len_buffer, fd); fprintf(tmp, "%s", buffer);
        //printf("[4]buffer is: %s\n",buffer);
        ok = 1;
      }
      if (ok == 1 && atoi(id)==idClient)
      {
        //printf("AM AJUNS IN EGALITATEA \n");
        while (strstr(buffer, "</train>") == NULL)// && strcmp(buffer, "</trainList>") != 0)
        {
          getline(&buffer, &len_buffer, fd); 
          //printf("[5]buffer is: %s\n",buffer);
          if (strstr(buffer,"<delayDeparture>") != NULL)
          {
            //atunci valoarea trebuie schimbata in 0
            strcpy(buffer,"				<delayDeparture>0</delayDeparture>\n");
          }
          if (strstr(buffer,"<delayArrival>") != NULL)
          {
            //atunci valoarea trebuie schimbata in 0
            strcpy(buffer,"				<delayArrival>0</delayArrival>\n");
          }
          if (strstr(buffer,"<earlyArrival>") != NULL)
          {
            //atunci valoarea trebuie schimbata in 0
            strcpy(buffer,"				<earlyArrival>0</earlyArrival>\n");
          }
          fprintf(tmp, "%s", buffer);
        }
      }
      ok=0;
    }
    fclose(fd);
    fclose(tmp);
    fd = fopen("trenuri.xml","w");
    tmp = fopen("xml.tmp","r");
    while(getline(&buffer, &len_buffer, tmp) != -1)
    {
      fprintf(fd,"%s",buffer);
    }
    fclose(fd);
    fclose(tmp);
  }
  else if (alegere == 3)     //avem un tren devreme, trebuie citit si early
  {
    //mai bagam o citire, cate minute mai devreme
    minute=readFromClient(arg); //acum avem si minutelw cu care intarzie

    bool ok = 0;     

    getline(&buffer, &len_buffer, fd); // parsing the xml until discovering the category
    //printf("[1]buffer is: %s\n",buffer);
    fprintf(tmp, "%s", buffer);
    while (strcmp(buffer, "</trainList>") != 0)
    {
      // parsing the xml until discovering the category
      //printf("am ajuns aixi\n");
      getline(&buffer, &len_buffer, fd);  fprintf(tmp, "%s", buffer);
      //printf("[2]buffer is: %s\n",buffer);
      if (strstr(buffer, "<type>") != NULL)
      {
        strncpy(type, buffer + 8, 2);
        getline(&buffer, &len_buffer, fd); fprintf(tmp, "%s", buffer);
        //printf("[3]buffer is: %s\n",buffer);
        strncpy(id, buffer + 6, 4);
        getline(&buffer, &len_buffer, fd); fprintf(tmp, "%s", buffer);
        //printf("[4]buffer is: %s\n",buffer);
        ok = 1;
      }
      if (ok == 1 && atoi(id)==idClient)
      {
        //printf("AM AJUNS IN EGALITATEA \n");
        while (strstr(buffer, "</train>") == NULL)// && strcmp(buffer, "</trainList>") != 0)
        {
          getline(&buffer, &len_buffer, fd); 
          //printf("[5]buffer is: %s\n",buffer);
          if (strstr(buffer,"<earlyArrival>") != NULL)
          {
            //atunci valoarea trebuie schimbata in 0
            strcpy(buffer,"				<earlyArrival>");
            changeITOA(intarziere,minute);
            //printf("intarziere este: %s\n", intarziere);
            //sleep(2);
            strcat(buffer,intarziere);
            strcat(buffer,"</earlyArrival>\n");
          }
          fprintf(tmp, "%s", buffer);
        }
      }
      ok=0;
    }
    fclose(fd);
    fclose(tmp);
    fd = fopen("trenuri.xml","w");
    tmp = fopen("xml.tmp","r");
    while(getline(&buffer, &len_buffer, tmp) != -1)
    {
      fprintf(fd,"%s",buffer);
    }
    fclose(fd);
    fclose(tmp);
  }
  
  //printf("[LINE 1470]A IESIT DIN searchXMLinfoClient\n");
  //fclose(fd); !!!!!de pus la celelalte doua
  strcpy(mesaj,"Informatiile au fost actualizate cu succes! Va multumim pentru asteptare!\n");
}

char *findCity(int nr) /*aici nr este numarul orasului, care se gasesc in ordine in orase.txt*/
{
  FILE *file;
  file = fopen("orase.txt", "r");
  char *buffer = NULL;
  size_t len_buffer = 0;

  for (int i = 1; i <= nr; i++)
    getline(&buffer, &len_buffer, file);
  // printf("Orasul citit este: %s\n",buffer);
  return buffer;
  fclose(file);
}
 
void changeITOA(char val[10], int nr)
{
  int i = 0;
  char aux[10]="";
  while (nr!=0)
  {
    aux[i]=nr%10+'0';
    //printf("aux[%d] = %s\n",i,aux);
    i++;
    nr/=10;
  }
  //printf("i este %d\n",i);
  for (int p=i-1,j=0;p>=0;p--,j++)
    val[j]=aux[p];
  //printf("val este: %s\n", val);
}
    

/*void fereastraGrafica()
{
  initscr();

  // print to screen
  printw("Hello World");

  // refreshes the screen
  refresh();

  // pause the screen output
  getch();

  // deallocates memory and ends ncurses
  endwin();
}*/

void sendToClient(char mesaj[100], void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  if (write(tdL.cl, &mesaj, 100) <= 0)
  {
    printf("[Thread %d] ", tdL.idThread);
    perror("[Thread]Eroare la write() catre client.\n");
  }
  else
    printf("[Thread %d]Mesajul a fost transmis cu succes.\n\n", tdL.idThread);
}

int readFromClient(void *arg)
{
  struct thData tdL;
  tdL = *((struct thData *)arg);
  int mesaj;
  /*primeste rapsunsul de la client*/
  if (read(tdL.cl, &mesaj, sizeof(int)) <= 0)
  {
    printf("[Thread %d]\n", tdL.idThread);
    perror("Eroare la read() de la client.\n");
  }
  // printf("MESAJUL PRIMIT ESTE este %d\n", mesaj);
  /*primeste rapsunsul de la client*/
  return mesaj;
}

int timpLocal()
{
  time_t now;
  struct tm *now_tm;
  int hour;

  now = time(NULL);
  now_tm = localtime(&now);
  hour = now_tm->tm_hour;
  return hour;
}