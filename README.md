# ESPHome Aldes EasyHome (iBUS)

Intégration **locale** d'une VMC **Aldes EasyHOME PureAIR Premium / Connect** dans **Home Assistant** avec **ESPHome**, sans AldesConnect Box ni cloud Aldes.

> **Version 1.1.0.** Intégration locale de la VMC via iBUS : mesures, diagnostics et commandes de mode depuis Home Assistant. Projet communautaire non officiel, indépendant d'Aldes.

## Origine du projet et montage électronique

Ce projet s'appuie sur les travaux de **Quentin57520**, **yanoooou**, **moi-même** et des contributeurs du forum Home Assistant Communauté Francophone (HACF) :

- [Discussion HACF : VMC Aldes EasyHOME PureAIR Premium → Aldes PureFlow MQTT](https://forum.hacf.fr/t/vmc-aldes-easyhome-pureair-premium-aldes-pureflow-mqtt/68269)
- [Échanges sur le PCB ESP32 réalisé par Quentin (page 10)](https://forum.hacf.fr/t/vmc-aldes-easyhome-pureair-premium-aldes-pureflow-mqtt/68269?page=10)

**Le montage électronique d'interface iBUS conçu par Quentin est nécessaire** : ne branchez **jamais** directement le bus iBUS de la VMC sur les GPIO 3,3 V de l'ESP32. Consultez [le message HACF n°214 consacré au PCB de Quentin](https://forum.hacf.fr/t/vmc-aldes-easyhome-pureair-premium-aldes-pureflow-mqtt/68269/214), ainsi que le schéma et les photos du montage publiés dans la discussion, et utilisez **la version correspondant à votre PCB**. Pour toute demande concernant l'obtention d'un PCB, rapprochez-vous de **Quentin57520** directement sur le forum HACF. Le dépôt fournit le logiciel ESPHome, pas un adaptateur électrique intégré à la carte ESP32.

La carte de commande VMC utilisée pour les essais porte le marquage **Aldes Himalaya01M1b**. Vérifiez la compatibilité de votre matériel avant installation.

### Matériel

- **ESP32-WROOM-32U ESP32 DevKitC V4**, avec antenne externe adaptée : [modèle utilisé pour les essais](https://www.amazon.fr/dp/B0D7ZGT9PM). Vérifiez la variante de carte livrée.
- **Carte d'interface iBUS / PCB de Quentin**, assemblée selon le schéma et les composants documentés dans [son message HACF n°214](https://forum.hacf.fr/t/vmc-aldes-easyhome-pureair-premium-aldes-pureflow-mqtt/68269/214). Pour obtenir un PCB, contactez **Quentin57520** sur le forum.
- Alimentation **5 V USB adaptée** pour l'ESP32, Wi-Fi 2,4 GHz et instance Home Assistant avec ESPHome.

**Attention :** la carte de commande de la VMC comporte des circuits secteur. Coupez l'alimentation avant toute intervention. Ne raccordez pas le secteur à l'interface ESP32 ; une alimentation USB séparée est préférable pour les premiers essais. Le bus iBUS n'est **ni un UART 3,3 V direct, ni du RS485/Modbus**.

### Raccordements logiques utilisés par le firmware

| Signal | GPIO ESP32 | Fonction |
| --- | --- | --- |
| Réception iBUS (RX) | **GPIO3** | Entrée via le circuit d'adaptation de Quentin (notamment R7 sur le PCB testé) |
| Émission iBUS (TX) | **GPIO1** | Sortie UART inversée vers l'interface iBUS |
| LED alimentation (Power) | **GPIO25** | Allumée en fonctionnement normal, éteinte en mode nuit |
| LED réception iBUS | **GPIO26** | Allumée 1 seconde après chaque trame VMC valide reçue, sauf en mode nuit |
| LED Wi-Fi / API | **GPIO27** | Indique l'état des connexions, sauf en mode nuit |
| Réservation UART secondaire | **GPIO32** | Broche réservée par le firmware pour l'attribution des UART ; pas une LED |

Paramètres iBUS : **2400 bauds, 8 bits, parité paire, 1 bit de stop (8E1)**. Le câblage physique dépend de la révision du PCB de Quentin : fiez-vous à son schéma.

### Trames iBUS : commandes et réponses de la VMC

Le composant ESPHome reconnaît les commandes émises (ou leurs échos) commençant par `FD 87` et les réponses de la VMC commençant par `87 00`. Le troisième octet contient la **longueur totale de la trame moins 1**. Toutes les positions ci-dessous sont des **indices à partir de zéro**.

#### Commande de mode vers la VMC — 18 octets

Trame de référence (Boost) : `FD 87 11 4B 02 02 00 00 65 FF 00 00 FF FF FF FF 4A 72`.

| Indice | Octet(s) | Rôle |
| --- | --- | --- |
| 0–1 | `FD 87` | En-tête de commande |
| 2 | `11` | Longueur moins 1 : 0x11 + 1 = 18 octets |
| 3–7 | `4B 02 02 00 00` | Champs fixes de la commande observée ; signification détaillée non établie |
| 8 | Variable | Code du mode demandé |
| 9–16 | `FF 00 00 FF FF FF FF 4A` | Champs fixes observés dans les commandes de mode |
| 17 | Variable | Checksum calculé sur les 17 octets précédents |

Codes de mode actuellement utilisés :

| Mode | Octet 8 |
| --- | --- |
| Programmation | `10` |
| Vacances | `21` |
| Quotidien | `43` |
| Boost | `65` |
| Invités | `87` |

L'ESP32 transmet la commande demandée sur le bus et la réémet périodiquement, environ toutes les 10 secondes. Le sélecteur Home Assistant reflète **le mode demandé**, pas un acquittement confirmé par la VMC.

#### Réponse standard de la VMC — 28 octets

Exemple de trame observée : `87 00 1B 4B 03 01 00 01 C0 03 87 00 00 00 72 44 76 64 73 64 50 02 17 E2 EE 02 4A D8`.

| Indice | Octet(s) | Interprétation actuelle |
| --- | --- | --- |
| 0–1 | `87 00` | En-tête de réponse VMC |
| 2 | `1B` | Longueur moins 1 : 0x1B + 1 = 28 octets |
| 3–5 | `4B 03 01` | Identifiant de réponse standard reconnu par le décodeur |
| 6–7 | Variable | Champs non décodés |
| 8–9 | Variable | PWM QAI 1, entier 16 bits *little-endian* |
| 10–13 | Variable | Champs non décodés |
| 14–15 | Variable | Température et humidité de la pièce 1 |
| 16–17 | Variable | Température et humidité de la pièce 2 |
| 18–19 | Variable | Température et humidité de la pièce 3 |
| 20–21 | Variable | CO₂ (ppm), entier 16 bits *little-endian* |
| 22 | Variable | Champ non décodé |
| 23 | Variable | Variation d'humidité (VarHR) |
| 24–25 | Variable | PWM QAI 2, entier 16 bits *little-endian* |
| 26 | Variable | Champ non décodé |
| 27 | Variable | Checksum |

Pour les températures, le décodeur applique la formule `(octet >> 2) - 6 + (octet & 3) × 0,25` en °C. L'humidité correspond à l'octet suivant, en pourcentage.

Les observations suggèrent que **PWM QAI 1** reflète la ventilation résultante et que **PWM QAI 2** représente un plancher de régulation de qualité de l'air. Cette interprétation reste expérimentale ; elle ne constitue pas une spécification officielle Aldes.

#### Réponses longues — 54 octets

Le composant accepte également les trames VMC valides de **54 octets**, en conserve la dernière et l'expose dans les diagnostics. Le détail de leurs champs n'est pas encore documenté de façon suffisamment fiable pour être présenté comme décodé.

#### Calcul et vérification du checksum

Le checksum est calculé de sorte que **la somme de tous les octets de la trame, checksum inclus, soit égale à zéro modulo 256**.

Pour une trame de `N` octets :

```text
checksum = (- somme(octets[0] à octets[N-2])) & 0xFF
validation : (somme(octets[0] à octets[N-1]) & 0xFF) == 0
```

Exemple : pour la commande Boost ci-dessus, le checksum est `0x72`. Le décodeur rejette les trames dont la somme n'est pas nulle, et ne publie pas les échos des commandes comme réponses VMC.

## Fonctionnalités actuelles

### Entités disponibles dans Home Assistant

Les entités sont présentées ci-dessous selon les rubriques de l'appareil ESPHome dans Home Assistant. Les noms des pièces proviennent des substitutions `piece_1`, `piece_2` et `piece_3`.

#### Contrôles

| Entité | Description |
| --- | --- |
| **iBUS Mode VMC** | Choix du mode : Programmation, Vacances, Quotidien, Boost ou Invités ; affiche la demande envoyée et non une confirmation de la VMC |
| **Mode nuit LEDs** | Interrupteur d'activation du mode nuit, désactivé par défaut |
| **Début mode nuit LEDs** | Heure de début du mode nuit, réglable à la minute ; valeur initiale 23:00 |
| **Fin mode nuit LEDs** | Heure de fin du mode nuit, réglable à la minute ; valeur initiale 07:00 |

#### Capteurs

| Entité | Description |
| --- | --- |
| **Température [pièce 1 à 3]** | Température de chacune des trois pièces configurées, en °C |
| **Humidité [pièce 1 à 3]** | Humidité relative de chacune des trois pièces, en % |
| **CO2** | Concentration en CO₂ reçue de la VMC, en ppm |
| **Variation humidite** | Indicateur VarHR transmis par la VMC |
| **PWM QAI 1 (brut)** | Valeur brute PWM QAI 1 remontée par la VMC |
| **PWM QAI 2 (brut)** | Valeur brute PWM QAI 2 remontée par la VMC |
| **QAI Index** | Estimation expérimentale de l'indice de qualité de l'air, de 0 à 100 |
| **Polluant dominant** | Contribution prédominante estimée : humidité ou CO₂ |

#### Diagnostics

| Entité | Description |
| --- | --- |
| **iBUS Connecté** | Indique si des réponses VMC valides sont reçues récemment |
| **iBUS RX brut hex** | Dernière réponse VMC valide en hexadécimal, sans écho TX |
| **iBUS Derniere trame VMC** | Dernière réponse VMC valide ; texte limité pour Home Assistant |
| **iBUS Derniere trame longue VMC** | Dernière réponse VMC valide de 54 octets |
| **iBUS Trames valides** | Compteur de trames VMC valides |
| **iBUS Checksums invalides** | Compteur d'échecs de checksum |
| **iBUS Erreurs de synchronisation** | Compteur d'erreurs de synchronisation |
| **iBUS Octets recus** | Compteur d'octets reçus |
| **QAI Humidite** | Copie de VarHR, utilisée comme contribution humidité |
| **QAI contribution CO2** | Contribution CO₂ estimée pour le calcul QAI |
| **Version firmware Aldes** | Version définie dans le package GitHub et compilée dans l'ESP32 |
| **Redemarrer Aldes EasyHome** | Bouton de redémarrage de l'ESP32 |

**À noter :** le calcul QAI est expérimental et non certifié par Aldes. Certains champs iBUS restent inconnus. Le capteur `self_controlled` n'est pas inclus dans cette version.

### Fonctions

#### Gestion du mode de la VMC

Home Assistant permet de demander les modes **Programmation**, **Vacances**, **Quotidien**, **Boost** et **Invités**. Le mode choisi est mémorisé sur l'ESP32, restauré après redémarrage et envoyé périodiquement sur iBUS. L'affichage du sélecteur représente **la commande demandée**, pas une confirmation d'exécution par la VMC.

#### Gestion des LEDs

| LED | Fonctionnement hors mode nuit |
| --- | --- |
| **Power — GPIO25** | Allumée lorsque l'ESP32 fonctionne |
| **iBUS RX — GPIO26** | S'allume pendant **1 seconde** à chaque réception d'une trame valide de la VMC ; une nouvelle réception prolonge l'allumage |
| **Wi-Fi / API — GPIO27** | Clignote pendant les connexions, puis reste allumée en continu lorsque Wi-Fi et API Home Assistant sont connectés |

Pour la LED **Wi-Fi / API**, les états sont les suivants :

| État | Comportement de la LED |
| --- | --- |
| Connexion au routeur Wi-Fi en attente | **1 seconde allumée / 1 seconde éteinte** |
| Wi-Fi connecté, connexion API Home Assistant en attente | **500 ms allumée / 500 ms éteinte** |
| Wi-Fi et API Home Assistant connectés | **Allumée fixe** |
| Connexion perdue | **Reprise du clignotement** correspondant à la connexion manquante |

Le **mode nuit LEDs** éteint les trois LEDs sur une plage horaire configurable dans Home Assistant. Il est désactivé par défaut ; ses horaires initiaux sont **23:00–07:00**. Les horaires et l'état de l'interrupteur sont conservés après redémarrage. La plage horaire utilise l'heure fournie par Home Assistant ; en l'absence d'heure valide, les LEDs restent autorisées. Les modifications des contrôles provoquent une réévaluation immédiate, avec une vérification périodique en complément.

#### Autres fonctions

- Intégration **locale** dans Home Assistant via l'API native ESPHome, sans cloud Aldes.
- Mise à jour du firmware en **OTA** après la première installation.
- Diagnostics de communication iBUS et conservation des dernières réponses valides.
- Capteur de version du firmware compilé ; la version n'est pas saisie dans le YAML de l'utilisateur et ne se met pas à jour sans nouveau flash.

## Installation dans ESPHome

1. Installez/configurez l'extension **ESPHome Device Builder** dans Home Assistant (ou utilisez votre ESPHome existant). Créez un **nouvel appareil ESP32**.
2. Ouvrez l'éditeur YAML de cet appareil. **Remplacez son contenu par celui de [`examples/aldes-easyhome.yaml`](examples/aldes-easyhome.yaml)**. Ne copiez pas directement `packages/easyhome.yaml` dans votre appareil : le fichier d'exemple l'importe automatiquement.
3. Dans la section `substitutions:`, personnalisez `device_name`, `friendly_name` et surtout **`piece_1`, `piece_2`, `piece_3`** selon les bouches de votre VMC.
4. Configurez les secrets ESPHome référencés par l'exemple : `wifi_ssid`, `wifi_password` et `aldes_easyhome_api_key`. La clé API doit être une **clé de chiffrement ESPHome valide** ; ne recopiez pas celle d'un autre appareil.
5. Vérifiez votre câblage, puis **validez et compilez** la configuration dans ESPHome. Pour le premier flash, privilégiez **l'USB**. Les mises à jour suivantes peuvent être réalisées en OTA.
6. Une fois l'ESP32 connecté au Wi-Fi, ajoutez ou acceptez son intégration **ESPHome** dans Home Assistant. Vérifiez les entités et les diagnostics iBUS avant d'essayer les changements de mode.

Extrait des paramètres à adapter :

```yaml
substitutions:
  device_name: aldes-easyhome
  friendly_name: Aldes EasyHome
  piece_1: Cuisine
  piece_2: Baignoire
  piece_3: Douche
```

Le fichier d'exemple complet configure également `esp32dev`, l'API native Home Assistant, l'OTA, le Wi-Fi et l'import du package :

```yaml
packages:
  easyhome: github://Fredzxda/esphome-aldes-easyhome/packages/easyhome.yaml@main
```

Les identifiants Wi-Fi et la clé API restent **dans la configuration ESPHome locale**, jamais dans ce dépôt public. L'exemple désactive les logs série UART0 (`logger: baud_rate: 0`) car ses broches servent au bus, et définit **`logger: level: ERROR`** pour n'afficher que les erreurs. Pour un dépannage ponctuel, remplacez `ERROR` par `DEBUG` dans votre YAML local puis recompilez.

## Structure du dépôt

- [`examples/aldes-easyhome.yaml`](examples/aldes-easyhome.yaml) : **fichier d'exemple à copier dans ESPHome**, contenant les substitutions et la configuration propre à l'appareil.
- [`packages/easyhome.yaml`](packages/easyhome.yaml) : **package partagé**, regroupant le protocole UART, les entités Home Assistant et la gestion des LEDs.
- [`components/aldes_ibus/`](components/aldes_ibus/) : **composant externe ESPHome** qui décode les réponses iBUS et émet les commandes de mode.

## Licence

GPL-3.0 — voir [LICENSE](LICENSE).
