# Reconstruction des humidités corrigées — essais et décision

Statut : **étude documentée, implémentation non réalisée sur `main`**. Données : export Home Assistant **janvier 2026** (humidités brutes `hrcu`, `hrba1`, `hrba2`, corrigées `hrcuco`, `hrba1co`, `hrba2co`, températures `tmpcu`, `tmpba1`, `tmpba2`). Les exports juin et septembre 2025 ne permettent pas la même comparaison directe des trois paires brut/corrigé.

## Résultats observés

Comparaisons synchrones exploitables en janvier : cuisine **496** (151 corrections), SDB1 **556** (28 corrections), SDB2 **580** (16 corrections), soit **1 632** comparaisons et **195** corrections.

- Jusqu'à **20 °C inclus**, les valeurs corrigées sont identiques aux valeurs brutes dans les comparaisons disponibles.
- Au-dessus de **20 °C**, les valeurs corrigées sont inférieures aux valeurs brutes ; corrections observées de **20,2 à 25,5 °C** selon les sondes.
- À température identique, le rapport `HR_corrigée / HR_brute` est identique à la précision flottante (dispersion maximale < 4 × 10⁻¹⁶ sur 21 températures distinctes). Le coefficient dépend donc de la température, et non de la pièce ni de l'humidité brute, sur cet échantillon.

| T (°C) | Coefficient mesuré |
|---:|---:|
| 20,0 | 1 (pas de correction) |
| 20,2 | 0,987523200061236 |
| 20,5 | 0,969119673128765 |
| 21,0 | 0,939257675283464 |
| 22,0 | 0,882440298500923 |
| 23,0 | 0,829266810783971 |
| 24,0 | 0,779479469813437 |
| 25,5 | 0,710634888680219 |

## Formules testées

La forme générale est `HR_corr = HR_brute × K(T)` pour `T > 20 °C`, sinon `HR_corr = HR_brute`.

1. Rapport des pressions de vapeur saturante (Magnus, 17,67 / 243,5) : erreur absolue moyenne **0,1073** point, maximum **0,4091** sur 195 corrections.
2. Clausius-Clapeyron simplifiée, `K(T) = exp(5423 × (1/(T+273.15) - 1/293.15))` : erreur moyenne **0,0114** point, maximum **0,0383**.
3. **Reconstruction continue empirique retenue** :
   ```text
   si T <= 20 : HR_corr = HR_brute
   sinon :
     K(T) = exp(32.770308 * (20/(481.1436+20) - T/(481.1436+T)))
     HR_corr = HR_brute * K(T)
   ```
   Ajustement antérieur rapporté sur janvier : erreur moyenne ≈ **0,0004** point, écart maximal < **0,002** point. **Ces coefficients sont empiriques, non confirmés comme formule officielle Aldes.** La formule exacte et les éventuels arrondis restent inconnus.

## Décision de conception

**Ne pas modifier la mesure brute utilisée par les calculs internes.** Ajouter sur une branche de développement `humidite-corrigee` un **interrupteur binaire Home Assistant** permettant à l'utilisateur de choisir **l'affichage des humidités brutes (défaut)** ou **corrigées** dans les **trois entités d'humidité existantes**. Conserver les mêmes noms et identifiants d'entités autant que possible pour ne pas casser les tableaux de bord.

Le sélecteur ne doit modifier **que les valeurs publiées vers Home Assistant**. Les calculs QAI (humidité excessive, air sec, polluant dominant, indice global), la variation d'humidité et toute logique de régulation ou diagnostic dépendant de l'humidité doivent continuer à utiliser les **valeurs brutes iBUS**, sans dépendre de l'interrupteur. Ne pas appliquer la correction dans le décodage ou le stockage des mesures brutes. Lors d'un changement de sélecteur, republier rapidement les trois valeurs affichées, sans modifier les valeurs sources.

## Validation requise sur la branche

- Vérifier la compilation ESPHome et le maintien des trois entités existantes.
- Tester T ≤ 20 °C (identité), T > 20 °C (correction), valeurs manquantes/NaN, basculement à chaud et restauration du sélecteur.
- Comparer les valeurs corrigées aux observations de janvier ; conserver une tolérance explicite, ne pas revendiquer une égalité parfaite.
- Comparer les résultats QAI et les autres calculs avant/après basculement : **aucun changement** à entrées iBUS identiques.
- Préserver `main` et la version stable `v1.2.0` pendant les essais.

## Limites

Janvier fournit les paires synchrones nécessaires. Les exports juin et septembre ne constituent pas une validation indépendante de la correction faute de mesures brutes et corrigées appariées. La formule peut être révisée si de nouvelles températures ou des relevés AldesConnect la contredisent.
