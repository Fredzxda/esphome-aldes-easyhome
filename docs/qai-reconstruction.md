# Reconstruction locale de l'indice QAI Aldes (26 octobre 2026)

## Objet et statut

Reproduire localement le `qai_index` auparavant fourni par le cloud Aldes, à partir des trames iBUS de la VMC. **Reconstruction empirique**, non spécification officielle. Aucune donnée météo externe n'est utilisée. Le code se trouve dans `components/aldes_ibus/aldes_ibus.h` et les entités dans `packages/easyhome.yaml`.

## Sources historiques et méthode

Exports d'états Home Assistant de **juin 2025**, **septembre 2025** et **janvier 2026**, contenant QAI cloud, CO₂, humidités brutes et, selon la période, humidités corrigées et polluant dominant. Les événements non simultanés ont été réalignés chronologiquement sur les dernières valeurs connues. Le barème humidité a été identifié sur septembre puis contrôlé sur juin et janvier. Le jeu de juin ne comporte pas tous les capteurs de cuisine : interpréter ses erreurs avec prudence.

Sur les **2 322** points de la comparaison globale précédente : **94,6 %** de prédictions exactes et erreur absolue moyenne **0,79 point** (septembre 765 points : 99,7 %, 0,003 ; janvier 672 : 90,0 %, 0,94 ; juin 885 : 95,4 %, 1,21). Ces chiffres proviennent de l'analyse historique, **pas d'une exécution du firmware**. Septembre a servi à identifier la formule : son score n'est pas une validation indépendante.

## Formules retenues

Avec `HR1`, `HR2`, `HR3` les humidités **brutes** des trois pièces (et non les `hr*co` corrigées) :

```text
H = floor(max(HR1, HR2, HR3))

QAI_humide =
  0                             si H <= 50
  floor(5*(H-50)/2)             si 51 <= H <= 60
  25                            si H = 61
  25 + floor(25*(H-61)/9)       si 62 <= H <= 70
  50                            si H = 71
  50 + floor(25*(H-71)/4)       si 72 <= H <= 75
  min(100, H-1)                si H >= 76

QAI_sec = clamp(floor(2.5*(50-min(HR1,HR2,HR3))), 0, 100)

QAI_humidite = max(QAI_humide, QAI_sec)
```

La pénalisation de l'air trop sec est observée en janvier (par exemple 44 % → QAI 15, 46 % → 10, 48 % → 5). Le plafonnement à 100 et les extrêmes non observés sont provisoires.

CO₂, en ppm :

```text
QAI_CO2_reel =
  0                         si CO2 <= 400
  (CO2-400)/16              si 400 < CO2 <= 1200
  50+(CO2-1200)/8           si 1200 < CO2 <= 1400
  75+(CO2-1400)/24          si CO2 > 1400

QAI_CO2 = clamp(ceil(QAI_CO2_reel)-1, 0, 100)
QAI_global = max(QAI_humidite, QAI_CO2)
```

Le CO₂ était précédemment étudié séparément (219 correspondances exactes sur 223). La convention d'arrondi ci-dessus suit cette hypothèse, à confirmer sur appareil.

## Anomalies et limites

- Janvier : quelques écarts, dont le 18/01 à 11:34 (QAI cloud 33 alors que la cuisine était à 86 %). Les événements suggèrent parfois une sélection différente de sonde ou une désynchronisation.
- Juin : certains QAI cloud à 100 ne sont pas expliqués, et la sonde de cuisine est absente de l'export.
- Pas de lissage temporel ajouté, faute de preuve suffisante.
- Les valeurs QAI sont calculées uniquement lorsque **les trois humidités et le CO₂** sont disponibles ; les contributions individuelles peuvent devenir disponibles avant le QAI global.
- En cas d'égalité, le texte `Polluant dominant` privilégie `Humidité`.
- La formule calcule un **indice de qualité de l'air**, pas une consigne de PWM. Elle ne modifie ni le mode VMC ni les commandes iBUS.
- Le capteur `Variation humidite` (VarHR) reste publié séparément ; il ne doit plus être confondu avec `QAI Humidite`.

## Vérifications à réaliser

1. Compiler le package sur la branche de développement et vérifier l'apparition des entités.
2. Vérifier sur l'appareil les valeurs limites : 50/51, 60/61/62, 70/71/72, 75/76 % et 400/1200/1400 ppm.
3. Comparer les contributions et l'indice global à de nouvelles observations réelles.
4. Ne fusionner dans `main` qu'après validation matérielle.
