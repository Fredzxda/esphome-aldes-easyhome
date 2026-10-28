# ESPHome Aldes EasyHome (iBUS)

Intégration **locale** d'une VMC **Aldes EasyHOME PureAIR Premium / Connect** dans **Home Assistant** avec **ESPHome**, sans AldesConnect Box ni cloud Aldes.

> Intégration locale de la VMC via iBUS : mesures, diagnostics et commandes de mode depuis Home Assistant. Projet communautaire non officiel, indépendant d'Aldes.

## Installation rapide

Pour commencer, vérifiez le [matériel et les précautions de sécurité](#matériel-et-sécurité), puis suivez le [guide d'installation ESPHome](#installation-dans-esphome). Le fichier d'exemple utilise la branche stable `main` ; une alternative commentée permet de tester une branche de développement.

## Ce que permet l'intégration

- Consulter les températures, humidités, CO₂ et indicateurs de qualité de l'air.
- Demander un mode de ventilation et consulter le mode réellement appliqué par la VMC.
- Observer la reprise de contrôle automatique liée à la qualité de l'air (`Self Controlled`).
- Régler l'extinction nocturne des LED et mettre à jour l'ESP32 en OTA.
- Diagnostiquer les communications iBUS, sans cloud Aldes.

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
  # Pour tester une branche de développement, remplacez la ligne ci-dessus par :
  # easyhome: github://Fredzxda/esphome-aldes-easyhome/packages/easyhome.yaml@dev/nom-de-branche
```

Les identifiants Wi-Fi et la clé API restent **dans la configuration ESPHome locale**, jamais dans ce dépôt public. L'exemple désactive les logs série UART0 (`logger: baud_rate: 0`) car ses broches servent au bus, et définit **`logger: level: ERROR`** pour n'afficher que les erreurs. Pour un dépannage ponctuel, remplacez `ERROR` par `DEBUG` dans votre YAML local puis recompilez.

### Entités disponibles dans Home Assistant

Les entités sont présentées ci-dessous selon les rubriques de l'appareil ESPHome dans Home Assistant. Les noms des pièces proviennent des substitutions `piece_1`, `piece_2` et `piece_3`.

#### Contrôles

| Entité | Description |
| --- | --- |
| **Mode VMC Demandé** | Sélecteur permettant de demander à la VMC un mode (Programmation, Vacances, Quotidien, Boost ou Invités). Il affiche la demande mémorisée et envoyée par l'ESP32, pas nécessairement le mode réellement appliqué. |
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
| **PWM Actuel** | Valeur PWM remontée par la VMC, représentative de sa ventilation effective (interprétation empirique). |
| **PWM Cible QAI** | Valeur PWM liée à la régulation de qualité de l'air ; ce n'est pas un minimum absolu applicable à tous les modes. |
| **QAI Index** | Estimation locale du QAI global : maximum des contributions humidité et CO₂ (0–100) |
| **Polluant dominant** | Contribution prédominante estimée : humidité ou CO₂ ; l'humidité est privilégiée en cas d'égalité. |
| **Mode VMC Actuel** | Mode effectivement déclaré par la VMC dans ses réponses iBUS. Il peut différer du mode demandé, notamment lorsque la régulation interne réagit à la qualité de l'air. |
| **Self Controlled** | Indique que la VMC a repris la maîtrise de sa ventilation en raison d'un besoin de qualité de l'air. L'état est actuellement déduit de `PWM Cible QAI >= 750` ; il ne distingue pas les niveaux de régulation. |

#### Diagnostics avancés

Ces entités servent principalement au dépannage et à l'analyse du protocole. Elles peuvent être ignorées pour une utilisation courante.

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
| **QAI Humidite** | Maximum des pénalités humidité élevée et air trop sec (humidité brute des 3 pièces) |
| **QAI humidite elevee** | Contribution de la plus forte humidité brute, barème non linéaire |
| **QAI air sec** | Contribution de la plus faible humidité brute lorsqu’elle est inférieure à 50 % |
| **QAI contribution CO2** | Contribution CO₂ selon le barème par plages et l’arrondi entier |
| **Version firmware Aldes** | Version définie dans le package GitHub et compilée dans l'ESP32 |
| **Redemarrer Aldes EasyHome** | Bouton de redémarrage de l'ESP32 |

**À noter :** le calcul QAI est une reconstruction expérimentale du QAI cloud Aldes, sans météo extérieure et sans lissage. Les valeurs de l’indice global attendent les trois sondes d’humidité et le CO₂. [Méthode, formules, statistiques et limites](docs/qai-reconstruction.md). Certains champs iBUS restent inconnus.

### Fonctions

#### Gestion du mode de la VMC

Home Assistant permet de demander les modes **Programmation**, **Vacances**, **Quotidien**, **Boost** et **Invités**. Le mode choisi est mémorisé sur l'ESP32, restauré après redémarrage et envoyé périodiquement sur iBUS. L'affichage du sélecteur **Mode VMC Demandé** représente la commande demandée, pas une confirmation d'exécution par la VMC. Le capteur **Mode VMC Actuel** est, lui, décodé à partir de la réponse iBUS de la VMC : il représente le mode qu'elle applique effectivement, lequel peut évoluer automatiquement en fonction de la qualité de l'air de la maison.

#### Reprise automatique de la ventilation : Self Controlled

Lorsque la qualité de l'air se dégrade, la régulation interne Aldes peut reprendre la main sur la vitesse de ventilation, même si Home Assistant a demandé un autre mode. Les observations suggèrent **deux niveaux de reprise de contrôle** :

- **Premier niveau** : la VMC impose une ventilation élevée, mais laisse encore le choix entre les deux modes rapides **Boost** et **Invités**.
- **Second niveau** : la VMC impose la vitesse maximale, qui semble correspondre au mode **Invités** ; cette correspondance reste à confirmer sur les trames iBUS.

Ces deux niveaux décrivent le comportement supposé de la régulation Aldes, **pas deux états actuellement exposés par ESPHome**. Le capteur binaire **Self Controlled** passe à `on` lorsque `PWM Cible QAI` atteint au moins **750**, seuil déduit des observations ; il ne mesure ni directement le QAI cloud ni le niveau de reprise de contrôle. La correspondance exacte entre les niveaux, les vitesses, les seuils et les codes iBUS reste à vérifier. Voir [les observations non élucidées](docs/observations-non-elucidees.md).

#### Gestion des LEDs

| LED | Fonctionnement hors mode nuit |
| --- | --- |
| **Power — GPIO25** | Allumée lorsque l'ESP32 fonctionne |
| **iBUS RX — GPIO26** | S'allume pendant **1 seconde** à chaque réception d'une trame valide de la VMC ; une nouvelle réception prolonge l'allumage |
| **Wi-Fi / API — GPIO27** | Clignote pendant les connexions, puis reste allumée en continu lorsque Wi-Fi et API Home Assistant sont connectés |

La LED Wi-Fi / API clignote en attente de connexion et reste allumée lorsque Wi-Fi et API Home Assistant sont connectés.

Le **mode nuit LEDs** éteint les trois LEDs sur une plage horaire configurable dans Home Assistant. Il est désactivé par défaut ; ses horaires initiaux sont **23:00–07:00**. Les horaires et l'état de l'interrupteur sont conservés après redémarrage. La plage horaire utilise l'heure fournie par Home Assistant ; en l'absence d'heure valide, les LEDs restent autorisées. Les modifications des contrôles provoquent une réévaluation immédiate.

#### Autres fonctions

- Intégration **locale** dans Home Assistant via l'API native ESPHome, sans cloud Aldes ni besoin d'AldesConnect Box.
- Mise à jour du firmware en **OTA** après la première installation.
- Diagnostics de communication iBUS et conservation des dernières réponses valides.
- Capteur de version du firmware.

## Matériel et sécurité

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

### Raccordements ESP32

| Signal | GPIO ESP32 | Fonction |
| --- | --- | --- |
| Réception iBUS (RX) | **GPIO3** | Entrée via le circuit d'adaptation de Quentin (notamment R7 sur le PCB testé) |
| Émission iBUS (TX) | **GPIO1** | Sortie UART inversée vers l'interface iBUS |
| LED alimentation (Power) | **GPIO25** | Allumée en fonctionnement normal, éteinte en mode nuit |
| LED réception iBUS | **GPIO26** | Allumée 1 seconde après chaque trame VMC valide reçue, sauf en mode nuit |
| LED Wi-Fi / API | **GPIO27** | Indique l'état des connexions, sauf en mode nuit |
| Réservation UART secondaire | **GPIO32** | Broche réservée par le firmware pour l'attribution des UART ; pas une LED |

Paramètres iBUS : **2400 bauds, 8 bits, parité paire, 1 bit de stop (8E1)**. Le câblage physique dépend de la révision du PCB de Quentin : fiez-vous à son schéma.

Pour les détails des commandes, réponses, codes de modes, offsets et checksums, consultez la [documentation technique du protocole iBUS](docs/protocole-ibus.md).

## Structure du dépôt

- [`examples/aldes-easyhome.yaml`](examples/aldes-easyhome.yaml) : **fichier d'exemple à copier dans ESPHome**, contenant les substitutions et la configuration propre à l'appareil.
- [`packages/easyhome.yaml`](packages/easyhome.yaml) : **package partagé**, regroupant le protocole UART, les entités Home Assistant et la gestion des LEDs.
- [`components/aldes_ibus/`](components/aldes_ibus/) : **composant externe ESPHome** qui décode les réponses iBUS, émet les commandes de mode et calcule le QAI local.
- [`docs/protocole-ibus.md`](docs/protocole-ibus.md) : **analyse technique des trames iBUS**, commandes, réponses, offsets et checksums.
- [`docs/qai-reconstruction.md`](docs/qai-reconstruction.md) : **journal de rétro-ingénierie QAI** : sources, formules, validations et anomalies.
- [`docs/observations-non-elucidees.md`](docs/observations-non-elucidees.md) : **observations et hypothèses non confirmées** sur les modes, les PWM et la reprise automatique de contrôle.

## Licence

GPL-3.0 — voir [LICENSE](LICENSE).
