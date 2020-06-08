/* includes ... */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno-base.h>
#include <linux/interrupt.h>

#define __user __attribute__((noderef, address_space(1)))


/*** Donnée propre à notre périphérique adxl345 ****/
struct adxl345_device {
    /* Données propres à un périphérique (exemple) */
    int irq_num;
    //int parameter_x;
    /* Le périphérique misc correspondant */
    struct miscdevice miscdev;
    char tampon [32][6];
};

static char adxl345_configuration(struct i2c_client *client){

    pr_info("debut de la fonction adxl345_fifo \n");
    char devID;
    char ModeMesure[2]   = {0x2D,0x08};//adresse du powerCTL/Mode mesure activée
    char ModeStandBy[2] = {0x2D,0x00};//adresse du powerCTL/Mode standBy activée
    char config_FIFO[2]  = {0x38,0x94};//Mode stream avec 0x20 samples
    char config_IRQ[2]   = {0x2E,0x02};//IRQ Watermark

    //Se mettre en mode stand By pour activer la fifo
    i2c_master_send(client,ModeStandBy,2);
    //Activer la fifo
    i2c_master_send(client,config_FIFO,2);
    //Configurer les IRQ
    i2c_master_send(client,config_IRQ,2);
    
    //Lire 32 fois dans la fifo à l'adresse addr dataX0 pour être sûr que la fifo soit vide
    int i;
    char buf = 0x00;
    char dataX0 = 0x32;
    for (i = 0 ; i < 32 ; i++){
        i2c_master_send(client, &dataX0, 1);
        i2c_master_recv(client, &buf, 1);
    }
    //Ma fifo est vide
    pr_info("Ma fifo est vide \n");

    //Je me remets en mode mesure
    i2c_master_send(client,ModeMesure,2);

    i2c_master_send(client,config_IRQ,2);

    char bufp = 0x00;
    i2c_master_send(client,&bufp,1);
    i2c_master_recv(client, &devID, 1);
    pr_info("fin de ma fonction fifo \n");

    return devID;
}

char read_fifo(struct i2c_client *client, char *tamp){

    int i;
    char REG_FIFO_STATUS = 0x39;
    char nb_value_fifo = 0;
    struct adxl345_device acc_dev;
    //char tamp [32*6];

    i2c_master_send(client,REG_FIFO_STATUS,1);
    i2c_master_recv(client,nb_value_fifo,1);

    
    for (i = 0 ; i < nb_value_fifo ; i++){
        i2c_master_send(client,0x32,1);
        i2c_master_recv(client,tamp,6);
    }

    return 1;
}


irqreturn_t adxl345_irq_handler(int irq_num, void *devID)
{
    /*interrupt-handling*/
    struct i2c_client *client = devID;
    struct adxl345_device acc_dev;
    char tamp [192];
    pr_info("Nous sommes dans l'IRQ handler \n");

    //Lire la fifo
    read_fifo(client,tamp);

    //Wake up puis remplit le tampon
    int j;
    int i;
    for (i = 0 ; i < 32; i++){

        for (j = 0 ; j < 6 ; j++)
            acc_dev.tampon[i][j] = tamp[(i*6)+j];
    }


    return IRQ_HANDLED;
}


int x;
/*** Définition de la fonction read permettant l'extraction des données du periph**/
/** à modifier : lit les valeurs dans le tampon puis wait à la fin **/
ssize_t accel_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos){
    
    struct miscdevice *md = (struct miscdevice *) file ->private_data;
    struct device *d = md->parent;
    struct i2c_client *client = container_of(d,struct i2c_client,dev);

    char add[6] = {0x32,0x33,0x33,0x34,0x35,0x36,0x37};
    char value[6];
    int i;

    i2c_master_send(client,add,1);
    i2c_master_recv(client,value,6);
    
    for (i = 0; i < 6 ; i ++)
        x = copy_to_user(buf, value[i],1);

    pr_info("value = %d \n",value);

    if (x < 0)
        pr_info("error \n");

    return 1;
}

long adxl345_ioctl(struct file *file, unsigned int cmd, unsigned long arg){

    pr_info("Appel de la fonction ioctl \n");
    return 1;
}


static int foo_probe(struct i2c_client *client,
                     const struct i2c_device_id *id)
{
    pr_info("Appel de probe\n");

    /*** Obtention de l'ID du periphérique ***/

    char buf = 0x00;
    i2c_master_send(client, &buf, 1);
    i2c_master_recv(client, &buf, 1);
    pr_info("le DEVID est : %d\n", buf);

    /******** Configuration du périphérique ******/

    //Output data rate 100Hz
    char buf1[2] = {0x2C,0x0A};
    i2c_master_send(client,buf1, 2);
    pr_info("Fréquence à 100Hz\n");

    //Toutes les interruptions désactivées
    char buf2[2] = {0x2E,0x00};
    i2c_master_send(client, buf2, 2);
    pr_info("Toutes les interruptions sont désactivées\n");

    //Format de données par défaut
    char buf3[2] = {0x31,0x00};
    i2c_master_send(client, buf3, 2);
    pr_info("Format de données par défaut\n");

    //Court-circuit de la FIFO(bypass)
    char buf4[2] = {0x38,0x00};
    i2c_master_send(client, buf4, 2);
    pr_info("Court-circuit de la FIFO(bypass)\n");

    //Mode mesure
    char buf5[2] = {0x2D,0x08};
    i2c_master_send(client, buf5, 2);
    i2c_master_recv(client, &buf, 1);
    pr_info("Mode mesure activé et le contenu du registre est %x \n", buf);

    /*
    int i2c_master_recv(struct i2c_client *client, char *buf, int count); 
    buf le buffer avec l'adresse de l'info à récupérer et count le nb d'octet ici =1
    */
         
    /**** Interface avec l'espace utilisateur ***/
    struct adxl345_device *acc_dev;
    struct file_operations *adxl345_fops;
    int ret;

    acc_dev = devm_kzalloc(&client->dev, sizeof(struct adxl345_device), GFP_KERNEL);

    adxl345_fops = (struct file_operations *) kzalloc(sizeof(struct file_operations), GFP_KERNEL);

    adxl345_fops->read=accel_read;
    adxl345_fops->unlocked_ioctl = adxl345_ioctl;

        if (!acc_dev)
        return -ENOMEM;

    /* Initialise la structure foo_device, par exemple avec les
       informations issues du Device Tree */
    acc_dev->irq_num = client->irq;
    //acc_dev->parameter_x = ...;

    /* Initialise la partie miscdevice de foo_device */
    acc_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
    acc_dev->miscdev.name = "adxl345";
    acc_dev->miscdev.fops = adxl345_fops;
    acc_dev->miscdev.parent = &client->dev;

    /* S'enregistre auprès du framework misc */
    i2c_set_clientdata(client,acc_dev);
    ret = misc_register(&acc_dev->miscdev);

    pr_info("la valeur de X est : %d \n",x);
    
    pr_info("bravo, le device est enregistré sur le framework misc\n");

    /* Mecanisme de gestion des interruptions via threadedIRQ */
    int a;
    adxl345_configuration(client);
    
    a = devm_request_threaded_irq(&client->dev,
                                    client->irq,
                                    NULL,
                                    adxl345_irq_handler,
                                    IRQF_ONESHOT,
                                    "adxl345",
                                    client);
    pr_info("threaded_irq = %d \n",a);


    return 0;
}

static int foo_remove(struct i2c_client *client)
{
    pr_info("Appel de remove \n");

    char buf6[2] = {0x2D,0x00};
    i2c_master_send(client, buf6, 2);
    pr_info("Mode standby activé\n");

    /***** Désenregistrement du device auprès du framework misc ****/
    struct adxl345_device *acc_dev;
    acc_dev = i2c_get_clientdata(client);
    misc_deregister(&acc_dev->miscdev);
    pr_info("Au revoir et à bientôt\n");

    return 0;
}


/* La liste suivante permet l'association entre un périphérique et son
   pilote dans le cas d'une initialisation STATIQUE sans utilisation de
   device tree.

   Chaque entrée contient une chaîne de caractère utilisée pour
   faire l'association et un entier qui peut être utilisé par le
   pilote pour effectuer des traitements différents en fonction
   du périphérique physique détecté (cas d'un pilote pouvant gérer
   différents modèles de périphérique).
*/
static struct i2c_device_id foo_idtable[] = {
    { "foo", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, foo_idtable);

#ifdef CONFIG_OF
/* Si le support des device trees est disponible, la liste suivante
   permet de faire l'association à l'aide du DEVICE TREE.

   Chaque entrée contient une structure de type of_device_id. Le champ
   compatible est une chaîne qui est utilisée pour faire l'association
   avec les champs compatible dans le device tree. Le champ data est
   un pointeur void* qui peut être utilisé par le pilote pour
   effectuer des traitements différents en fonction du périphérique
   physique détecté.
*/
static const struct of_device_id foo_of_match[] = {
    { .compatible = "ad,adxl345",
      .data = NULL },
    {}
};

MODULE_DEVICE_TABLE(of, foo_of_match);
#endif

static struct i2c_driver foo_driver = {
    .driver = {
        /* Le champ name doit correspondre au nom du module
           et ne doit pas contenir d'espace */
        .name   = "foo",
        .of_match_table = of_match_ptr(foo_of_match),
    },

    .id_table       = foo_idtable,
    .probe          = foo_probe,
    .remove         = foo_remove,
};

module_i2c_driver(foo_driver);

/* MODULE_LICENSE, MODULE_AUTHOR, MODULE_DESCRIPTION... */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pilote accelerator with TB");
MODULE_AUTHOR("WL");