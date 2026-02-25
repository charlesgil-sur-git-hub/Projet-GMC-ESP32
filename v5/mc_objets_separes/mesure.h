/**
 * @brief   Coder une classe  : 
 *                    Declaration de la classe mesure pour DAO
 * @file    	mesure.h
 * @author	cgil 
   @version	1.0
 * @date    fev 2026
 */
 
#ifndef MESURE_H
#define MESURE_H

#include <Arduino.h>

/** \class  	Mesure

 * \brief		Classe Mesure 

    base sur la table mesure 
        id_mesure INTEGER PRIMARY KEY AUTOINCREMENT
        date_creation TEXT DEFAULT (datetime('now', 'localtime'))
        valeur_tdc INTEGER
*/
 
class Mesure {
private:
        //! Attributs

        // identifiant primary unique
        int id_mesure;

        //! date de creation de lenreg
        String date_creation; 
        
        //! Valeur temperature en DIXIEMES de celcius (toujours en INT pas de virgules)
        int valeur_tdc; 
    
public:
        /**
        * \brief	Constructeurs de la classe 
                         parametrique ou pas
        */
        Mesure(int _id_mesure, String _date_creation, int _valeur_tdc);
        Mesure();

        /**
        * \brief	accesseurs
        */

        inline int getIdMesure () const {return id_mesure;};
        inline void setIdMesure (int _id_mesure)
            {id_mesure=_id_mesure;};

        inline String getDateCreation () const {return date_creation;};
        inline void setDateCreation (String _date_creation)
            {date_creation=_date_creation;};

        inline int getValeurTdc () const {return valeur_tdc;};
        inline void setValeurTdc (int _valeur_tdc)
            {valeur_tdc=_valeur_tdc;};

};

#endif
