/**
 * \brief Pour debugger

 *
 */

#ifndef DBG_H
#define DBG_H

/**
    Permet de mettre l'esp32 en mode Hybride sur 2 réseaux simultanés
*/
#define DEBUG_GMC_BOX_HYBRIDE

#ifdef DEBUG_GMC_BOX_HYBRIDE


// Activer le mode debug  
#define DEBUG_GMC_HOME_BOX 
//#define DEBUG_GMC_LABOINFO_BOX 
//#define DEBUG_GMC_S9_PARTAGE 


#ifdef DEBUG_GMC_HOME_BOX
	//! identifiants de Box (pour le mode Station)
    #define DEBUG_GMC_HOME_BOX_SSID "Freebox-34F871"
    #define DEBUG_GMC_HOME_BOX_PWD  ""
#endif


#ifdef DEBUG_GMC_LABOINFO_BOX
	//! identifiants de Box (pour le mode Station)
    #define DEBUG_GMC_LABOINFO_BOX_SSID "laboinfo"
    #define DEBUG_GMC_LABOINFO_BOX_PWD  "" // "IRIS for ever"
#endif


#ifdef DEBUG_GMC_S9_PARTAGE
	//! identifiants de Box (pour le mode Station)
    #define DEBUG_GMC_S9_PARTAGE_SSID "S9_cg"
    #define DEBUG_GMC_S9_PARTAGE_PWD  ""
#endif

#endif  // if DEBUG_GMC_BOX_HYBRIDE

#endif
