# Observations iBUS non élucidées

Ce journal rassemble les comportements **observés mais non expliqués** de la VMC Aldes EasyHome. Il ne constitue pas une spécification du protocole. À chaque découverte, conserver les trames brutes, l'heure, le contexte, les hypothèses et le degré de certitude. Ne déplacer une entrée vers la documentation du protocole qu'après validation.

## 1. Code de mode `0x99` et PWM Actuel = 1000

**Statut : confirmé comme observation ; signification inconnue.**

Le 25 octobre 2026, trois transitions brèves ont été observées dans les réponses VMC iBUS de **28 octets** (trames courtes). Elles contiennent toutes :
- octets d'index 8–9 : `E8 03`, soit PWM Actuel = **1000** (entier little-endian) ;
- octet d'index 10 : `99`, mode actuel inconnu `0x99`.

Trames intégrales récupérées dans l'historique Home Assistant :

**12:37:15 (heure locale France)**
```text
87 00 1B 4B 03 01 00 01 E8 03 99 00 00 00 74 49 74 54 72 55 6D 04 17 1B EE 02 4A 61
```
À 12:37:25, retour à PWM 750 / mode Boost (`0x65`).

**15:20:30**
```text
87 00 1B 4B 03 01 00 01 E8 03 99 00 00 00 6D 43 6A 4E 67 4D 83 02 17 18 90 01 4A DF
```
À 15:20:40, retour à PWM 180 / mode Vacances (`0x21`).

**15:33:47**
```text
87 00 1B 4B 03 01 00 01 E8 03 99 00 00 00 6D 43 69 4E 67 4D 59 02 17 18 90 01 4A 0A
```
À 15:33:57, retour à PWM 180 / mode Vacances (`0x21`).

**Questions ouvertes :** `0x99` est-il un mode transitoire, une phase de commande ou un autre état ? Pourquoi PWM = 1000 ? Quelle est la durée réelle de cet état entre deux lectures (~10 s) ?

## 2. Sens du registre PWM QAI2

**Statut : interprétation provisoire.**

Le champ auparavant nommé « PWM Min » est renommé **PWM Cible QAI** dans le package ESPHome pour ne pas lui attribuer une contrainte permanente non démontrée.

Observations du 25 octobre 2026 :
- En `self_controlled`, PWM QAI2 = 750 et PWM Actuel = 750 (mode Boost), y compris avec une consigne Vacances.
- Après désactivation de `self_controlled`, PWM QAI2 = 400 mais PWM Actuel = 180 en mode Vacances.
- Donc PWM QAI2 **n'est pas un minimum absolu en mode manuel** ; il pourrait être une cible QAI ou un minimum conditionnel lorsque la régulation automatique est active.

**À vérifier :** comparer les deux PWM pendant plusieurs cycles, différents modes demandés, et à l'activation/désactivation de `self_controlled`.

## 3. Déclenchement de `self_controlled` et calcul du QAI

**Statut : observations de reprise de contrôle ; seuils et niveaux exacts non confirmés.**

L'utilisateur avait constaté sur le modèle/cloud Aldes que `self_controlled` s'active lorsque **QAI > 69**. Il ne faut pas assimiler ce QAI cloud à notre indice local sans comparaison.

**Ancienne hypothèse abandonnée (ne correspond pas au firmware actuel) :**
```text
QAI_Index = min(100, max(0, VarHR, 0.065 * CO2 - 28))
```

**Calcul actuellement implémenté :**
```text
QAI_humidite = max(QAI_humidite_elevee, QAI_air_sec)
QAI_Index = max(QAI_humidite, QAI_CO2)
```
Les contributions sont calculées à partir des **trois humidités brutes** et du **CO₂**, selon les barèmes et arrondis détaillés dans [la reconstruction QAI](qai-reconstruction.md). `VarHR` est publié séparément et **n'entre pas dans ce calcul**. L'indice local est expérimental et ne doit pas être assimilé sans validation à celui du cloud Aldes.

**Self Controlled dans le firmware :** capteur binaire dérivé de `PWM Cible QAI >= 750`. Il indique une reprise de contrôle supposée par la régulation de qualité de l'air, mais **ne distingue pas les deux niveaux** de reprise observés ou envisagés :
- Niveau 1 : ventilation élevée avec possibilité de choisir entre **Boost** et **Invités** (à confirmer).
- Niveau 2 : vitesse maximale imposée, qui pourrait correspondre à **Invités** (à confirmer).

La relation exacte entre les deux niveaux, les seuils QAI, `PWM Cible QAI` et `Mode VMC Actuel` n'est pas encore établie.

Événement du 25 octobre 2026 :
- 11:29:23 : QAI_Index local atteint 100.
- 11:30:46 : PWM QAI2 passe de 400 à 750 et `self_controlled` passe à `on`.
- Contexte : douche ; humidité Douche augmente plus rapidement que Baignoire, jusqu'à 100 %.

**Questions ouvertes :** formule exacte du QAI cloud ; rôle de VarHR, CO2, humidité, éventuel étalonnage et temporisation ; seuil(s) et hystérésis.

**Piste de recherche :** récupérer les anciennes valeurs cloud enregistrées via `aldes2mqtt` dans les sauvegardes historiques Home Assistant et comparer des séries temporelles synchronisées. Sauvegarde d'add-on « Aldes 2 MQTT 0.2.0 » datée du 09/11/2025 ; plusieurs sauvegardes complètes avec base de données en 2025–2026. Archives non encore extraites.

## 4. Trames longues VMC

**Statut : absence d'observation, pas preuve d'absence sur le bus.**

Le 25 octobre 2026, le capteur Home Assistant `sensor.aldes_easyhome_ibus_derniere_trame_longue_vmc` est resté `unknown` (hors périodes `unavailable` liées aux reconnexions ESP32). Aucune trame longue publiée dans son historique sur la journée examinée.

**À vérifier :** si des trames longues sont effectivement émises, si le parseur les reconnaît et si le capteur est alimenté correctement.

---

## Méthode pour les nouvelles observations

Pour chaque cas, noter :
1. Date, heure locale, version du firmware et branche Git.
2. Action réalisée et contexte physique.
3. Trame brute complète et longueur, si disponibles.
4. Valeurs Home Assistant avant / pendant / après.
5. **Faits confirmés**, **hypothèses** et **tests restant à faire** séparément.

Ne pas modifier les automatisations de diagnostic uniquement sur la base d'une hypothèse non validée.
