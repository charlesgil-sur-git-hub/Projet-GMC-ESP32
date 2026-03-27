/**
 * @brief   Coder une classe  : 
 *                    Code  de la classe mesure pour DAO
 * @file    	mesure.h
 * @author	cgil 
   @version	1.0
 * @date    fev 2026
 */
 
 #include "mesure.h"


/** \brief		Classe Mesure 
  

    base sur la table mesure 
        id_mesure INTEGER PRIMARY KEY AUTOINCREMENT
        date_creation TEXT DEFAULT (datetime('now', 'localtime'))
        valeur_tdc INTEGER 
*/
 
/**
 * @brief constructeur
 */
Mesure::Mesure(int _id_mesure, String _date_creation, int _valeur_tdc) \
    : id_mesure(_id_mesure), date_creation(_date_creation), valeur_tdc(_valeur_tdc) 
{
 
}

Mesure::Mesure() 
{}

 
