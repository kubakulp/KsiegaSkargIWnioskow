#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>

key_t shmkey;
key_t semkey;
int   shmid;
int   semid;
char *ptr;
int pojemnosc;

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
	printf("\n[Serwer]: dostalem SIGINT => koncze i sprzatam...");
	/* Odlaczenie pamieci wspolnej oraz jej usuniecie, usuniecie zbioru semaforow */
	printf(" (odlaczenie shm: %s, usuniecie shm: %s usuniecie sem: %s)\n",
			(shmdt(shared_data) == 0)        ?"OK":"blad shmdt",
			(shmctl(shmid, IPC_RMID, 0) == 0)?"OK":"blad shmctl",
			(semctl(semid, IPC_RMID, 0) == 0)?"OK":"blad semctl");
	exit(0);
}

/* Obsluga SIGTSTP */
void sgnhandle2(int signal)
{
    int pusto=1; /* Zmienna informujaca czy ksiega jest pusta */
    for(int i=0; i<pojemnosc; i++) /* Sprawdzam czy jakikolwiek wpis posiada nazwe uzytkownika */
    {
        if(*(shared_data -> nazwaUzytkownika)=='\0')
        {

        }
        else
        {
            pusto = 0;
        }
    }
    /* Wypisanie zawartosci ksiegi */
    if(pusto==1)
    {
        printf("\nKsiega skarg i wnioskow jest jeszcze pusta\n");
    }
    else
    {
	    printf("\n__________Ksiega skarg i wnioskow: __________\n");
        for(int i=0; i<pojemnosc; i++)
        {
            if(*shared_data[i].nazwaUzytkownika=='\0')
            {

            }
            else
            {
                printf("\33[2K\r[%s]: %s\n", shared_data[i].nazwaUzytkownika, shared_data[i].txt);
            }
        }
    }
}

int main(int argc, char * argv[])
{
	struct shmid_ds buf;
    pojemnosc = strtol(argv[1],&ptr,10);
    /* Obsluga sygnalow */
	signal(SIGINT, sgnhandle1);
	signal(SIGTSTP, sgnhandle2);
    /* Sprawdzenie ilosci parametrow */
    if(argc!=3)
    {
        printf("Uruchom serwer z dwoma parametrami gdzie:\n");
        printf("Pierwszy jest liczba calkowita n oznaczajaca pojemnosc ksiegi\n");
        printf("Drugi ktory jest liczba calkowita oznaczajaca maksymalny rozmiar pojedynczego wpisu w bajtach\n");
        exit(1);
    }
    else
    {
        printf("[Serwer]: ksiega skarg i wnioskow (WARIANT A)\n");
    }

    /* Sprawdzenie poprawnosci podanych parametrow */
    int i;
    char *next;
    for (i = 1; i < argc; i++)
    {
        strtol (argv[i], &next, 10);
        if ((next == argv[i]) || (*next != '\0') || strtol(argv[i],&ptr,10)<0)
        {
            printf ("parametr '%s' nie jest liczba calkowita dodatnia\n", argv[i]);
            printf("Uruchom serwer z dwoma parametrami gdzie:\n");
            printf("Pierwszy jest liczba calkowita dodatnia n oznaczajaca pojemnosc ksiegi\n");
            printf("Drugi ktory jest liczba calkowita dodatnia oznaczajaca maksymalny rozmiar pojedynczego wpisu w bajtach\n");
		    exit(1);
        }
    }

    /* Tworzenie kluczy */
	printf("[Serwer]: tworze klucze na podstawie pliku %s...", argv[0]);
    if( (shmkey = ftok(argv[0], 1)) == -1 )
    {
	    printf("Blad tworzenia klucza dla pamieci wspolnej!\n");
		exit(1);
	}
    if( (semkey = ftok(argv[0], 2)) == -1 )
    {
	    printf("Blad tworzenia klucza dla semaforow!\n");
		exit(1);
	}
	printf(" OK (klucze: shm: %ld, sem: %ld)\n", shmkey, semkey);

    /* Tworzenie pamieci wspolnej */
	printf("[Serwer]: tworze segment pamieci wspolnej dla ksiegi na %s wpisow po %sb...\n", argv[1], argv[2]);
	if( (shmid = shmget(shmkey, sizeof(struct my_data)*pojemnosc, 0600 | IPC_CREAT | IPC_EXCL)) == -1)
    {
		printf(" blad shmget!\n");
		exit(1);
	}
	shmctl(shmid, IPC_STAT, &buf);
	printf(" OK (id: %d, rozmiar: %zub)\n", shmid, buf.shm_segsz);

	/* Dolaczenie pamieci wspolnej do stworzonej struktury */
	printf("[Serwer]: dolaczam pamiec wspolna...");
    shared_data = (struct my_data *) shmat(shmid, (void *)0, 0);
	if(shared_data == (struct my_data *)-1)
    {
		printf(" blad shmat!\n");
		exit(1);
	}
    shared_data -> iloscWpisow = pojemnosc;
    shared_data -> iloscPozostalychWpisow = pojemnosc;
	printf(" OK (adres: %lX)\n", (long int)shared_data);

	/* Tworzenie semaforow */
	printf("[Serwer]: tworze semafory... ");
	if( (semid = semget(semkey, pojemnosc, 0600 | IPC_CREAT | IPC_EXCL)) == -1)
    {
		printf(" blad semget!\n");
		exit(1);
	}
	printf(" OK (id: %d)\n", semid);

	/* Nadanie zmiennej semaforowej wartosci 1 dla kazdego z semaforow */
    for(int i=0; i<pojemnosc; i++)
    {
        if(semctl(semid, i, SETVAL, 1) == -1)
        {
            printf(" blad przy nadaniu wartosci semaforowi %d!\n", i);
		    exit(1);
        }
    }

    printf("[Serwer]: nacisnij Ctrl^Z by wyswietlic stan ksiegi\n");
	printf("[Serwer]: nacisnij Ctrl^C by zakonczyc program\n");

    while(8)
    {

    }

	return 0;
}

