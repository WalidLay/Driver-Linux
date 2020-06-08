#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(){
    int fd;
    int buf;
    int r;

    fd = open("/dev/adxl345", O_RDONLY);
    r = read(fd,&buf,1);

    printf("Axe X0 = %d \n", buf[0]);
    printf("Axe X1 = %d \n", buf[1]);
    printf("Axe Y0 = %d \n", buf[2]);
    printf("Axe Y1 = %d \n", buf[3]);
    printf("Axe Z0 = %d \n", buf[4]);
    printf("Axe Z1 = %d \n", buf[5]);

    printf("open = %d \n", fd);
    printf("read = %d \n", r);

    close(fd);

    return 0;
}

/***
 if (dev/adxl345 != NULL)
    {
        // Boucle de lecture des caractères un à un
        do
        {
            caractereActuel = fgetc(fichier); // On lit le caractère
            printf("%c", caractereActuel); // On l'affiche
        } while (caractereActuel != EOF); // On continue tant que fgetc n'a pas retourné EOF (fin de fichier)
 
        fclose(dev/adxl345);
    }

***/