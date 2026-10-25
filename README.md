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

- **ESP32-WROOM-32U ESP32 DevKitC V4**, avec kit d'antennes externes, modèle acheté pour ce projet : [Amazon France — B0D7ZGT9PM](https://www.amazon.fr/dp/B0D7ZGT9PM). Vérifiez la variante livrée : **WROOM-32U** utilise une antenne externe adaptée, à installer sur son connecteur RF.
- **Carte d'interface iBUS / PCB de Quentin**, assemblée suivant le schéma et les composants documentés sur le forum HACF. Pour vous procurer un PCB, contactez **Quentin57520** sur le forum.
- Alimentation **5 V USB adaptée** pour l'ESP32, réseau Wi-Fi 2,4 GHz et instance Home Assistant avec ESPHome.

**Attention :** la carte de commande de la VMC comporte des circuits secteur. Coupez l'alimentation avant toute intervention. Ne raccordez pas le secteur à l'interface ESP32 ; l'alimentation USB séparée est préférable pour les premiers essais. Le bus iBUS n'est **ni un UART 3,3 V direct, ni du RS485/Modbus**.

### Raccordements logiques utilisés par le firmware

| Signal | GPIO ESP32 | Remarque |
| --- | --- | --- |
| Réception iBUS (RX) | **GPIO3** | Via le circuit d'adaptation de Quentin, notamment R7 sur le PCB testé |
| Émission iBUS (TX) | **GPIO1** | Signal UART inversé dans le package ESPHome |
| LED activité réception | **GPIO26** | Selon montage du PCB |
| LED état Wi-Fi / API | **GPIO27** | Selon montage du PCB |
| GPIO25 | Réservé à UART | Ne pas le réutiliser comme sortie LED |

Paramètres iBUS : **2400 bauds, 8 bits, parité paire, 1 bit de stop (8E1)**. Le câblage physique dépend de la révision de la carte de Quentin : fiez-vous à son schéma, pas uniquement aux noms des GPIO.

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

## Fonctionnalités actuelles

- Lecture des trames VMC iBUS de **28 et 54 octets** avec vérification du checksum.
- Remontée de températures et humidités des **trois pièces configurées**, du CO₂ et de données QAI/PWM issues des trames décodées.
- Sélection de modes VMC : **Programmation, Vacances, Quotidien, Boost, Invités**. La sélection représente la **commande demandée**, pas nécessairement un accusé de réception de la VMC.
- Diagnostics : connexion iBUS, nombre de trames valides, checksums invalides, erreurs de synchronisation, octets reçus.
- `iBUS RX brut hex` : dernière **réponse VMC valide**, sans l'écho de la commande émise par l'ESP32. Malgré son nom historique, ce capteur ne montre pas les octets invalides.
- `iBUS Derniere trame VMC` : dernière trame VMC valide, quelle que soit sa longueur.
- `iBUS Derniere trame longue VMC` : conserve spécifiquement la dernière trame VMC valide de **54 octets**.

**Limite connue :** les champs de la trame longue sont encore étudiés ; certaines mesures QAI sont des estimations calculées et non des valeurs officielles Aldes. La transmission de commandes doit être vérifiée sur chaque installation.

## Structure du dépôt

- [`examples/aldes-easyhome.yaml`](examples/aldes-easyhome.yaml) : **point de départ à copier dans ESPHome**.
- [`packages/easyhome.yaml`](packages/easyhome.yaml) : configuration commune, entités et UART.
- [`components/aldes_ibus/`](components/aldes_ibus/) : composant externe ESPHome pour le protocole iBUS.

## Version et branche

La branche **`main`** contient la version **1.1.0** décrite ici. Les développements futurs sont réalisés séparément sur `dev/ibus-rx`.

## Version 1.1.0 — inventaire des entités

### Pilotage

- Sélecteur **iBUS Mode VMC** : Programmation, Vacances, Quotidien, Boost, Invités. La commande est transmise sur iBUS et répétée périodiquement ; l'état affiché est le mode demandé, **pas une confirmation de la VMC**.
- Bouton de diagnostic **Redemarrer Aldes EasyHome** : redémarre l'ESP32.

### Capteurs des trois pièces

Les noms des pièces sont définis par `piece_1`, `piece_2` et `piece_3` dans le fichier YAML de l'utilisateur :

- **Température** et **Humidité** pour chacune des trois pièces (six entités).

### Qualité de l'air et régulation

- **CO2** (ppm).
- **Variation humidite**.
- **PWM QAI 1 (brut)** et **PWM QAI 2 (brut)**.
- **QAI Humidite** (diagnostic, reprise de la variation d'humidité).
- **QAI contribution CO2** (diagnostic, valeur estimée).
- **QAI Index** (estimation bornée entre 0 et 100, non certifiée par Aldes).
- **Polluant dominant** (texte, comparaison des contributions humidité / CO₂).

### Diagnostics iBUS

- **iBUS Connecté** (état de réception de trames valides).
- **iBUS RX brut hex** (dernière réponse VMC valide, sans écho TX).
- **iBUS Derniere trame VMC** (dernière trame valide).
- **iBUS Derniere trame longue VMC** (dernière trame valide de 54 octets).
- **iBUS Trames valides**, **iBUS Checksums invalides**, **iBUS Erreurs de synchronisation**, **iBUS Octets recus** (compteurs).

### Autres fonctions

- Décodage des trames de 28 et 54 octets avec contrôle du checksum ; interface UART iBUS 2400 bauds 8E1.
- LEDs GPIO26 (activité RX) et GPIO27 (état Wi-Fi / API).
- Connexion Home Assistant native via ESPHome, Wi-Fi et mise à jour OTA.
- Mémorisation du mode demandé sur l'ESP32 après redémarrage.

**Points à connaître pour la v1.1.0 :** décodage de certains octets des trames longues incomplet ; calcul QAI estimatif, non validé par Aldes ; compatibilité testée principalement avec la carte Aldes Himalaya01M1b et le montage de Quentin ; absence de retour d'acquittement fiable du mode VMC. Vérifiez les commandes sur votre propre installation avant tout usage non supervisé.

## Licence

GPL-3.0 — voir [LICENSE](LICENSE).


## Gestion des LEDs et version du firmware

La branche `main` ajoute :

- Un capteur de diagnostic Home Assistant **Version firmware Aldes**, indiquant la version définie dans le package GitHub et compilée dans l'ESP32 (actuellement `1.1.0` dans cette version). La valeur **ne se met pas à jour à distance** : elle change après compilation et installation d'un nouveau firmware.
- Trois contrôles Home Assistant : **Mode nuit LEDs** (interrupteur désactivé par défaut), **Début mode nuit LEDs** (23:00 par défaut) et **Fin mode nuit LEDs** (07:00 par défaut). Les deux horaires sont réglables à la minute et conservés après redémarrage. Les LEDs sont éteintes uniquement si le mode nuit est activé et si l'heure Home Assistant est dans la plage configurée. Si l'heure n'est pas disponible, elles restent autorisées.
- GPIO25 réservé à la LED d'alimentation, GPIO26 à l'activité RX, GPIO27 à l'état Wi-Fi/API. La réservation UART secondaire passe sur GPIO32. Vérifier la polarité et le comportement réel de la LED GPIO25 sur le PCB avant validation.

**Important :** l'association automatique du numéro de version aux tags GitHub nécessite une étape de publication supplémentaire (workflow GitHub Actions et génération de la configuration avant compilation). Le capteur actuel reflète une valeur définie dans le package GitHub, et non dans le YAML personnel de l'utilisateur. Cette valeur reste à synchroniser automatiquement avec les tags lors du processus de publication ; elle ne lit pas les tags GitHub en temps réel.

Pour tester la branche, importer `packages/easyhome.yaml@main` et son exemple associé. La branche `main` et les tags publiés ne sont pas modifiés.
