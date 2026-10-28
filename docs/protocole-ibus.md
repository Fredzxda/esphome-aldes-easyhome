# Protocole iBUS — notes de rétro-ingénierie

Ce document conserve les trames, offsets, codes et calculs identifiés lors de l'analyse des échanges entre l'AldesConnect Box et la VMC. **Documentation communautaire non officielle** : les interprétations expérimentales sont signalées et peuvent évoluer.

Retour au [guide utilisateur](../README.md). Voir aussi [les observations non élucidées](observations-non-elucidees.md) et [la reconstruction du QAI](qai-reconstruction.md).

# Trames iBUS : commandes et réponses de la VMC

Le composant ESPHome reconnaît les commandes émises (ou leurs échos) commençant par `FD 87` et les réponses de la VMC commençant par `87 00`. Le troisième octet contient la **longueur totale de la trame moins 1**. Toutes les positions ci-dessous sont des **indices à partir de zéro**.

#### Commande de mode vers la VMC — 18 octets

Exemple de trame (Mode Boost demandé) : `FD 87 11 4B 02 02 00 00 65 FF 00 00 FF FF FF FF 4A 72`.

| Indice | Octet(s) | Rôle |
| --- | --- | --- |
| 0–1 | `FD 87` | En-tête de commande |
| 2 | Variable\* | Longueur de la trame moins 1 : 0x11 + 1 = 18 octets |
| 3 | `4B` | Pattern de début de payload |
| 4–7 | `02 02 00 00` | Champs fixes de la commande observée ; signification détaillée non établie |
| 8 | Variable | Code du mode demandé |
| 9–15 | `FF 00 00 FF FF FF FF` | Champs fixes observés dans les commandes de mode |
| 16 | `4A` | Pattern de fin de payload |
| 17 | Variable | Checksum calculé sur les 17 octets précédents |

\* Le mode Vacances n'est pour l'instant que partiellement implémenté. Si les plages horaires et les modes sont communiqués vers la VMC, les trames pourraient être plus longues. Pour l'instant, seule la valeur 11, correspondant à des commandes de 18 octets, a été observée.

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
| 3 | `4B` | Pattern de début de payload |
| 4–5 | `03 01` | Identifiant de réponse standard reconnu par le décodeur |
| 6–7 | Variable | Champs non décodés |
| 8–9 | Variable | PWM QAI 1, entier 16 bits *little-endian* |
| 10 | Variable | Mode de la VMC |
| 11–13 | `00 00 00` | Champs non décodés |
| 14–15 | Variable | Température et humidité de la pièce 1 |
| 16–17 | Variable | Température et humidité de la pièce 2 |
| 18–19 | Variable | Température et humidité de la pièce 3 |
| 20–21 | Variable | CO₂ (ppm), entier 16 bits *little-endian* |
| 22 | `17` | Champ non décodé |
| 23 | Variable | Variation d'humidité (VarHR) |
| 24–25 | Variable | PWM QAI 2, entier 16 bits *little-endian* |
| 26 | `4A` | Pattern de fin de payload |
| 27 | Variable | Checksum |

Pour les températures, le décodeur applique la formule `(octet >> 2) - 6 + (octet & 3) × 0,25` en °C. L'humidité correspond à l'octet suivant, en pourcentage.

Les observations suggèrent que **PWM QAI 1** reflète la ventilation résultante et que **PWM QAI 2** représente un plancher de régulation de qualité de l'air. Cette interprétation reste expérimentale ; elle ne constitue pas une spécification officielle Aldes.

#### Réponses longues — 54 octets

Le composant accepte également les trames VMC valides de **54 octets**, en conserve la dernière et l'expose dans les diagnostics. Le détail de leurs champs n'est pas encore documenté de façon suffisamment fiable pour être présenté comme décodé. Ces trames sont principalement envoyés par la VMC lors de son démarrage puis de manière non régulière et beaucoup plus rare par la suite.

#### Calcul et vérification du checksum

Le checksum est calculé de sorte que **la somme de tous les octets de la trame, checksum inclus, soit égale à zéro modulo 256**.

Pour une trame de `N` octets :

```text
checksum = (- somme(octets[0] à octets[N-2])) & 0xFF
```

Exemple : pour la commande Boost ci-dessus, le checksum est `0x72`. Le décodeur rejette les trames dont la somme n'est pas nulle, et ne publie pas les échos des commandes comme réponses VMC.

