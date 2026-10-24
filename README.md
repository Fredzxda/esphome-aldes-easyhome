# ESPHome Aldes EasyHome (iBUS)

Intégration **locale** d'une VMC **Aldes EasyHOME PureAIR Premium / Connect** dans **Home Assistant** avec **ESPHome**, sans AldesConnect Box ni cloud Aldes.

> **Projet expérimental — branche `dev/ibus-rx`.** Réception des trames iBUS, remontée de mesures et envoi périodique de commandes de mode sont implémentés, mais le pilotage et le décodage restent en cours de validation. Ce projet n'est pas une intégration officielle Aldes.

## Origine du projet et montage électronique

Ce projet s'appuie sur les travaux de **Quentin57520**, **yanoooou** et des contributeurs du forum Home Assistant Communauté Francophone (HACF) :

- [Discussion HACF : VMC Aldes EasyHOME PureAIR Premium → Aldes PureFlow MQTT](https://forum.hacf.fr/t/vmc-aldes-easyhome-pureair-premium-aldes-pureflow-mqtt/68269)
- [Échanges sur le PCB ESP32 réalisé par Quentin (page 10)](https://forum.hacf.fr/t/vmc-aldes-easyhome-pureair-premium-aldes-pureflow-mqtt/68269?page=10)

**Le montage électronique d'interface iBUS conçu par Quentin est nécessaire** : ne branchez **jamais** directement le bus iBUS de la VMC sur les GPIO 3,3 V de l'ESP32. Consultez le schéma et les photos du montage publiés dans la discussion HACF, et utilisez **la version correspondant à votre PCB**. Le dépôt fournit le logiciel ESPHome, pas un adaptateur électrique intégré à la carte ESP32.

La carte de commande VMC utilisée pour les essais porte le marquage **Aldes Himalaya01M1b**. Vérifiez la compatibilité de votre matériel avant installation.

### Matériel

- **ESP32-WROOM-32U ESP32 DevKitC V4**, avec kit d'antennes externes, modèle acheté pour ce projet : [Amazon France — B0D7ZGT9PM](https://www.amazon.fr/dp/B0D7ZGT9PM). Vérifiez la variante livrée : **WROOM-32U** utilise une antenne externe adaptée, à installer sur son connecteur RF.
- **Carte d'interface iBUS / PCB de Quentin**, assemblée suivant le schéma et les composants documentés sur le forum HACF.
- Alimentation **5 V USB adaptée** pour l'ESP32, réseau Wi-Fi 2,4 GHz et instance Home Assistant avec ESPHome.

**Attention :** la carte de commande de la VMC comporte des circuits secteur. Coupez l'alimentation avant toute intervention. Ne raccordez pas le secteur à un montage expérimental ; l'alimentation USB séparée est préférable pour les premiers essais. Le bus iBUS n'est **ni un UART 3,3 V direct, ni du RS485/Modbus**.

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
  easyhome: github://Fredzxda/esphome-aldes-easyhome/packages/easyhome.yaml@dev/ibus-rx
```

Les identifiants Wi-Fi et la clé API restent **dans la configuration ESPHome locale**, jamais dans ce dépôt public. L'exemple désactive les logs série UART0 (`logger: baud_rate: 0`) car ses broches servent au bus.

## Fonctionnalités actuelles

- Lecture des trames VMC iBUS de **28 et 54 octets** avec vérification du checksum.
- Remontée de températures et humidités des **trois pièces configurées**, du CO₂ et de données QAI/PWM issues des trames décodées.
- Sélection de modes VMC : **Programmation, Vacances, Quotidien, Boost, Invités**. La sélection représente la **commande demandée**, pas nécessairement un accusé de réception de la VMC.
- Diagnostics : connexion iBUS, nombre de trames valides, checksums invalides, erreurs de synchronisation, octets reçus.
- `iBUS RX brut hex` : dernière **réponse VMC valide**, sans l'écho de la commande émise par l'ESP32. Malgré son nom historique, ce capteur ne montre pas les octets invalides.
- `iBUS Derniere trame VMC` : dernière trame VMC valide, quelle que soit sa longueur.
- `iBUS Derniere trame longue VMC` : conserve spécifiquement la dernière trame VMC valide de **54 octets**.

**Limite connue :** les champs de la trame longue sont encore étudiés ; certaines mesures QAI sont expérimentales. La transmission de commandes doit être vérifiée sur chaque installation.

## Structure du dépôt

- [`examples/aldes-easyhome.yaml`](examples/aldes-easyhome.yaml) : **point de départ à copier dans ESPHome**.
- [`packages/easyhome.yaml`](packages/easyhome.yaml) : configuration commune, entités et UART.
- [`components/aldes_ibus/`](components/aldes_ibus/) : composant externe ESPHome pour le protocole iBUS.

## Branches

- `main` : branche principale ; ne pas supposer qu'elle inclut les dernières fonctions.
- `dev/ibus-rx` : branche de développement utilisée dans ce guide, avec réception, diagnostics et émission expérimentale de commandes.

## Licence

GPL-3.0 — voir [LICENSE](LICENSE).
