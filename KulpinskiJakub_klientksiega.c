#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<signal.h>

key_t shmkey;
key_t semkey;
int   shmid;
int   semid;
char  buf[100];
int zajety = -1; /* zmienna uzywana do podnoszenia semafora w przypadku gdy ten zostal opuszczony lecz klient nie dodal wpisu i opuscil program przy pomocy sygnalu SIGINT */

/* Struktura danych przechowywana w pamieci wspolnej */
struct my_data
{
    char txt[100];
    char nazwaUzytkownika[50];
    long iloscWpisow;
    long iloscPozostalychWpisow;
}*shared_data;

/* Obsluga SIGINT */
void sgnhandle1(int signal)
{
	printf("\n[Klient]: dostalem SIGINT => koncze...\n");
    if(zajety>=0)
    {
	    printf("[Klient]: podnoszenie semafora...\n");
        if(semctl(semid, zajety, SETVAL, 1) == -1)
        {
		    printf("Blad podnoszenia semafora!\n");
		    exit(1);
        }
        printf("(OK)\n");
    }
	exit(0);
}

int main(int argc, char * argv[])
{
	signal(SIGINT, sgnhandle1);    /* Obsluga sygnalu */

    /* Sprawdzenie ilosci parametrow */
    if(argc!=3)
    {
        printf("Uruchom klienta z dwoma parametrami gdzie:\n");
        printf("Pierwszy jest nazwa pliku sluzaca do generowania klucza IPC\n");
        printf("Drugi ktory jest nazwa uzytkownika\n");
        exit(1);
    }

    /* Tworzenie kluczy */
	if( (shmkey = ftok(argv[1], 1)) == -1)
    {
		printf("Blad tworzenia klucza dla pamieci wspolnej!\n");
		exit(1);
	}

	if( (semkey = ftok(argv[1], 2)) == -1)
    {
		printf("Blad tworzenia klucza dla semaforow!\n");
		exit(1);
	}

	/* dolaczenie uprzednio utworzonego przez serwer segmentu pamieci dzielonej*/
	if( (shmid = shmget(shmkey, 0, 0)) == -1 )
    {
		printf(" blad shmget\n");
		exit(1);
	}
	shared_data = (struct my_data *) shmat(shmid, (void *)0, 0);
	if(shared_data == (struct my_data *)-1)
    {
		printf(" blad shmat!\n");
		exit(1);
	}

	if( (semid = semget(semkey, shared_data -> iloscWpisow, 0600)) == -1 )
    {
		printf(" blad semget\n");
		exit(1);
	}

	/* Obsluga klienta gdy ksiega jest juz zapelniona */
    if(shared_data -> iloscPozostalychWpisow == 0)
    {
        printf("W ksiedze nie ma juz wolnego miejsca!\n");
        shmdt(shared_data); /* odlaczenie pamieci wspolnej */
        exit(0);
    }

    int j=0; /* zmienna ktora odpowiada za wypisanie komunikatu kiedy oczekujemy na wolny semafor */
    int i=0;

    while(8)
    {
        for(i=0; i<shared_data -> iloscWpisow; i++)
        {
            if(semctl(semid, i, GETVAL, 0) != 1)
            {
                if(shared_data -> iloscPozostalychWpisow == 0)
                {
                    printf("W ksiedze nie ma juz wolnego miejsca!\n");
                    shmdt(shared_data); /* odlaczenie pamieci wspolnej */
                    exit(0);
                }
            }
            else
            {
                semctl(semid, i, SETVAL, 0); /* opuszczenie semafora */
                zajety = i;
                printf("Klient ksiegi skarg i wnioskow wita!\n");
                printf("[Wolnych %ld wpisow (na %ld)]\n",shared_data -> iloscPozostalychWpisow, shared_data -> iloscWpisow);
	            printf("Napisz co ci doskwiera:\n");
	            /* wpisywanie do pamieci dzielonej */
                fgets(buf, 100, stdin);
                buf[strlen(buf)-1] = '\0';
                strcpy(shared_data[shared_data->iloscWpisow-shared_data->iloscPozostalychWpisow].txt, buf);
                strcpy(shared_data[shared_data->iloscWpisow-shared_data->iloscPozostalychWpisow].nazwaUzytkownika, argv[2]);
	            (shared_data -> iloscPozostalychWpisow)--;
	            printf("Dziekuje za dokonanie wpisu do ksiegi\n");
	            shmdt(shared_data); /* odlaczenie pamieci wspolnej */
                exit(0);
            }

            if(i+1>=shared_data -> iloscWpisow)
            {
                i=0;
                if(j==0)
                {
                    printf("[Klient]: Oczekuje na wolny semafor\n");
                }
                j++;
            }
        }
    }
	return 0;
}
