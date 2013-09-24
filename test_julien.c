/*
	Test libusb - ASTUPS
	Auteur: Julien Lechalup�
	But: Tester et apprendre le fonctionnement de la libusb
	afin d'�crire une librairie de communication USB
*/

#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENDPOINT_TO_ARDUINO 0x83
#define ENDPOINT_FROM_ARDUINO 0x04

// Affiche toute les informations d'un p�riph�rique USB
void afficherInfoUSB(libusb_device* usb);

// Prompt de choix du p�riph�rique � utiliser
int choixPeripherique(const int max);

// Affiche la configuration active du p�riph�rique
void afficherConfigUSB(libusb_device* usb,libusb_device_handle* usb_com);

// D�tache le driver syst�me pour r�cup�rer la main sur le p�riph�rique
void detacherDriver(libusb_device_handle* usbCom);

// Reserve l'interface de communication du p�riph�rique pour la prochaine 
// Action d'entr�e/sortie
void reserverInterface(libusb_device_handle* usb);

int main()
{
	// Variables
	libusb_context* contexte = NULL; // Contexte d'ex�cution de la libusb
	libusb_device** liste = NULL; // Liste des p�riph�riques USB disponible
	libusb_device* arduino = NULL; // P�riph�rique ouvert
	libusb_device_handle* arduino_com = NULL; // Communication avec le p�rph�rique
	
	int erreur = 0; // Retour d'erreurs
	int tailleListe; // Taile de la liste de p�riph�riques
	int i; // Compteur
	int octetsRecu = 0; // Octets recu depuis l'arduino
	
	char tampon[64]; // Tampon des donn�es lues
	
	
	printf("=== Test libusb ===\nInitialisation de la libusb.\n");
	
	// Intilialisation de la libusb
	erreur = libusb_init(&contexte);
	// Param�trage de la verbosit� (pr�cision de l'affichage DEBUG)
	libusb_set_debug(contexte,3);
	// V�rification des erreurs
	if ( erreur != 0)
	{
		printf("Erreur d'initialisation:\n> %s\nArret du programme.\n",libusb_error_name(erreur));
		exit(-1);
	}
	
	// Affichage des P�riph�riques USB disponible
	// R�cup�ration de la liste
	tailleListe = libusb_get_device_list(contexte,&liste);
	// Parcour de la liste et affichage
	printf("------------------------------------\n");
	for ( i = 0 ; i < tailleListe ; i++ )
	{
		printf("-> Choix n %d\n",i);
		afficherInfoUSB(liste[i]);
		printf("------------------------------------\n");
	}
	
	// Choix du p�riph�rique
	arduino = liste[choixPeripherique(tailleListe-1)];
	
	// Ouverture de la communication avec l'arduino
	erreur = libusb_open(arduino,&arduino_com);
	// V�rification des erreurs
	if ( erreur != 0 )
	{
		printf("Erreur d'ouverture du peripherique:\n> %s\nArret du programme.\n",libusb_error_name(erreur));
		exit(-1);
	}
	
	// Affichage des information coml�mentaire du p�riph�rique
	afficherConfigUSB(arduino,arduino_com);
	
	// V�rification du driver
	detacherDriver(arduino_com);
	
	// Reservation de l'interface
	reserverInterface(arduino_com);
	
	// Envoie des donn�es � l'arduino
	bzero(tampon,64);
	printf("Qu'envoyer a l'arduino ?\n-> ");
	scanf("%s",tampon);
	erreur = libusb_bulk_transfer(arduino_com, ENDPOINT_TO_ARDUINO,tampon,strlen(tampon),&octetsRecu,150);
	if ( erreur != 0 )
	{
		printf("Erreur d'envoie des donnees.\n -> %s\n",libusb_error_name(erreur));
		perror("PERROR");
	}
	else if ( octetsRecu != strlen(tampon) )
	{
		printf("Erreur d'envoie de donnees:\n-> donnees envoyees: \"%s\"\n-> taille de donnees: %d\n-> taille des donnee envoyees %s\n",tampon,strlen(tampon),octetsRecu);
		perror("PERROR");
	}
	printf("Donnees envoyees %d octets, veuillez patienter.\n",octetsRecu);
	
	// Lecture des donn�es re�u provenant de l'arduino 
	// Mise � z�ro du buffer
	bzero(tampon,64);
	octetsRecu = 0;
	
	// Reservation des interfaces
	//reserverInterface(arduino_com);
	
	// Lecture des donn�es envoy�es
	erreur = libusb_bulk_transfer(arduino_com, ENDPOINT_FROM_ARDUINO,tampon,64,&octetsRecu,150);
	if ( erreur != 0 )
	{
		printf("Erreur de lecture des donnees.\n -> %s\n",libusb_error_name(erreur));
		perror("PERROR");
	}
	else
	{
		// Affichage des donn�es
		printf("Donnees recu.\n");
		printf("Taille des donn�es: %d octets\n",octetsRecu);
		printf("Contenu du tampon:\n%s\n",tampon);
		printf("------------------------------------\n");
	}
	
	// Lib�ration de l'interface
	erreur = libusb_release_interface(arduino_com,1);
	if ( erreur != 0 )
	{
		printf("Erreur de liberation de l'interface 1:\n> %s\nArret du programme.\n",libusb_error_name(erreur));
		exit(-1);
	}
	
	libusb_close(arduino_com);
	
	// Arr�t de la libusb
	printf("Arret de la libusb.\nFin du programme.\n\n");
	libusb_free_device_list(liste,tailleListe);
	libusb_exit(contexte);
	
	return 0;
}

// Affiche toute les informations d'un p�riph�rique USB
void afficherInfoUSB(libusb_device* usb)
{
	// Variables
	struct libusb_device_descriptor info; // Informations sur le p�riph�rique USB
	int erreur; // Retour d'erreur
	char vitesse[32]; // Chaine de caract�re repr�sentant la vitesse
	
	// R�cup�ration des informations
	erreur = libusb_get_device_descriptor(usb,&info);
	// Test des erreurs
	if ( erreur != 0 )
	{
		printf("-> Erreur lors de la lecture d'information du peripherique %u: \"%s\".\n",(unsigned int)usb,libusb_error_name(erreur));
		return;
	}
	
	// R�cup�ration de la vitesse
	switch(libusb_get_device_speed(usb))
	{
		case 0:
			strcpy(vitesse,"inconnue");
			break;
		case 1:
			strcpy(vitesse,"low");
			break;
		case 2:
			strcpy(vitesse,"full");
			break;
		case 3:
			strcpy(vitesse,"high");
			break;
		case 4:
			strcpy(vitesse,"super");
			break;
	}
	
	// Affichage des donn�es
	printf("-> Peripherique %u - Informations.\n",libusb_get_device_address(usb));
	printf("-> Bus n %u\n",libusb_get_bus_number(usb));
	printf("-> Vitesse: %s\n",vitesse);
	printf("-> USB version : 0x%x\n",info.bcdUSB);
	printf("-> ID vendeur : %d\n",info.idVendor);
	printf("-> ID produit : %d\n",info.idProduct);
	printf("-> Version du peripherique : 0x%x\n",info.bcdDevice);
}

// Prompt de choix du p�riph�rique � utiliser
int choixPeripherique(const int max)
{
	// Variable
	int choix; // Nombre entr�
	char buffer[64]; // Tampon
	
	printf("Avec quel peripherique voulez vous communiquer ? [0-%d]\n-> ",max);
	
	// Boucle de saisie
	do
	{
		// Saisie
		gets(buffer);
		
		// V�rification de l'entr�e
		if ( sscanf(buffer,"%d",&choix) != 1 )
		{
			choix = -1;
			printf("\nErreur de saisie, entrez un nombre. [0-%d]\n->",max);
		}
		
	} while ( !(choix >= 0 && choix <= max) );
	
	return choix;
}

// Affiche la configuration active du p�riph�rique
void afficherConfigUSB(libusb_device* usb,libusb_device_handle* usb_com)
{
	// Variables
	struct libusb_device_descriptor info; // Information du p�riph�rique
	char produit[64]; // Nom du produit
	char vendeur[64]; // Nom du vendeur
	char serial[64]; // Num�ro de s�rie
	int erreur = 0; // Retour d'erreur;
	
	// R�cup�ration des informations
	erreur = libusb_get_device_descriptor(usb,&info);
	// Test des erreurs
	if ( erreur != 0 )
	{
		printf("-> Erreur lors de la lecture d'information du peripherique %u: \"%s\".\n",(unsigned int)usb,libusb_error_name(erreur));
		return;
	}
	
	// R�cup�ration des chaines
	libusb_get_string_descriptor_ascii(usb_com,info.iProduct,produit,64);
	libusb_get_string_descriptor_ascii(usb_com,info.iManufacturer,vendeur,64);
	libusb_get_string_descriptor_ascii(usb_com,info.iSerialNumber,serial,64);
	
	// Affichage
	printf("------------------------------------\n");
	printf("-> Informations complementaire du peripherique.\n");
	printf("-> Nom du produit: %s\n",produit);
	printf("-> Nom du vendeur: %s\n",vendeur);
	printf("-> Numero de serie: %s\n",serial);
	printf("------------------------------------\n");
}

// D�tache le driver syst�me pour r�cup�rer la main sur le p�riph�rique
void detacherDriver(libusb_device_handle* usbCom)
{
	int erreur = 0;
	#ifndef WIN32
		// V�rification du driver
		// A ne pas faire sur Windows
		erreur = libusb_kernel_driver_active(usbCom,1);
		if ( erreur )
		{
			// D�tachement du driver pour r�server l'interface
			erreur = libusb_detach_kernel_driver(usbCom,1);
			if ( erreur != 0 )
			{
				printf("Erreur de detachement du driver:\n> %s\nArret du programme.\n",libusb_error_name(erreur));
				exit(-1);
			}
		}
		else if ( erreur != 0 )
		{
			printf("Erreur de verification du driver:\n> %s\nArret du programme.\n",libusb_error_name(erreur));
			exit(-1);
		}
		printf("Driver detache !\n------------------------------------\n");
	#else
		printf("Action inutile sous Windows !\n");
	#endif
}

// Reserve l'interface de communication du p�riph�rique pour la prochaine 
// Action d'entr�e/sortie
void reserverInterface(libusb_device_handle* usb)
{
	// Variable
	int erreur = 0;
	
	erreur = libusb_claim_interface(usb,0);
	if ( erreur != 0 )
	{
		printf("Erreur de reservation de l'interface 0\n> %s\nArret du programme.\n",libusb_error_name(erreur));
		exit(-1);
	}
	
	erreur = libusb_claim_interface(usb,1);
	if ( erreur != 0 )
	{
		printf("Erreur de reservation de l'interface 1\n> %s\nArret du programme.\n",libusb_error_name(erreur));
		exit(-1);
	}
}
