/**
 * \brief classe de traitement au debut (on)
			de la fonction setup
 *
 * \file : setup_on.cpp
 * \date : 2026
 *
 */

#ifndef SETUP_ON_H
#define SETUP_ON_H

#include <Arduino.h>


/** 
 *  @brief : classe SetupOn 
		cette methode est execute au debut 
		du setup de l'arduino
*/
class SetupOn {
private:
   
public:
	/**
	 * @brief constructeur// destructeur INLINE
	 */
	SetupOn() {};
	~SetupOn() {};

    /**
    * @brief synchroniser horloge avec le micro controleur
    */
	bool synchroniserHorlogeAuSetup();
	
		
	/**
	 * @brief preparer le moniteur serie au debut
	 */
	void preparerMoniteurAuSetup(Stream &SerialHS);

};

#endif
