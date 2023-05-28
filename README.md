# Column Generation Solver

A solver based on column generation.

![columngeneration](img/columngeneration.jpg?raw=true "columngeneration")

[image source](https://commons.wikimedia.org/wiki/File:ColonnesPavillonTrajan.jpg)

## Description

The goal of this repository is to provide a simple framework to quickly implement heuristic algorithms based on column generation.

It is only required to provide the description of the linear program of the Dantzig–Wolfe decomposition of the master problem as well as the algorithm solving the pricing problem.
No branching rule implementation is required.

The currently implemented algorithms are based on the algorithms from "Primal Heuristics for Branch and Price: The Assets of Diving Methods" (Sadykov et al., 2019).

This package does not implement any exact algorithm. However, if the pricing algorithm is exact, it provides a valid dual bound.
If the pricing algorithm is heuristic, the primal algorithms still works, but then the dual bound is not valid.

Solving a problem only requires a couple hundred lines of code (see examples).

A linear programming solver is required. Currently, CLP, Xpress and CPLEX are supported.

Features:
* Algorithms:
  * Column generation `column_generation`
  * Greedy `greedy`
  * Limited Discrepancy Search `limited_discrepancy_search`
  * Heuristic Tree Search `heuristic_tree_search`
* Sabilization technics:
  * Static and self-adjusting Wentges smoothing
  * Static and automatic directional smoothing

## Examples

Data can be downloaded from [fontanf/orproblems](https://github.com/fontanf/orproblems)

When the sub-problems can be solved with a very efficient algorithm - typically a pseudo-polynomial dynamic programming algorithm - then the bottleneck is the resolution of the linear problems. This is the case for the examples Cutting Stock, Multiple Knapsack, Generalized Assignment and Star Observation Scheduling.

When the sub-problems are more difficult to solve, their resolution become the bottleneck of the algorithm. This is the case for the examples Geometrical Variable-sized Bin Packing, Bin Packing with Conflicts, Capacitated Vehicle Routing, Vehicle Routing Problem with Time Windows and Graph Coloring. Here, these sub-problems are solved using generic approaches based on Heuristic Tree Search or Local Search. During the first column generation iterations, these heuristic algorithms are stopped early to avoid spending a lot of time to find trivial columns.

### Packing

[Cutting Stock Problem](examples/cuttingstock.hpp)
* Pricing problem: Bounded Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

<details><summary>Benchmarks</summary>
<p>

* Benchmarks:
  * `python3 ../optimizationtools/optimizationtools/bench_run.py --main ./bazel-bin/examples/cuttingstock_main --csv ../ordata/cuttingstock/data.csv -l cuttingstock -a "heuristic_tree_search" -t 60`

</p>
</details>

[Multiple Knapsack Problem](examples/multipleknapsack.hpp)
* Pricing problem: Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

<details><summary>Benchmarks</summary>
<p>

* Benchmarks:
  * `python3 ../optimizationtools/optimizationtools/bench_run.py --main ./bazel-bin/examples/multipleknapsack_main --csv ../ordata/multipleknapsack/data.csv -l multipleknapsack -a "heuristic_tree_search" -t 10`

</p>
</details>

Generalized Assignment Problem from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver/blob/master/generalizedassignmentsolver/algorithms/columngeneration.cpp)
* Pricing problem: Knapsck Problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

Geometrical Cutting Stock and Variable-sized Bin Packing Problems from [fontanf/packingsolver](https://github.com/fontanf/packingsolver/blob/master/packingsolver/algorithms/column_generation.hpp)
* Pricing problem: Geometrical Knapsack Problems solved with the algorithms from the same repository

[Bin Packing Problem with Conflicts](examples/binpackingwithconflicts.hpp)
* Pricing problem: Knapsack Problem with Conflicts solved with the [Heuristic Tree Search](https://github.com/fontanf/treesearchsolver/blob/main/examples/knapsackwithconflicts.hpp) algorithm from [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Routing

[Capacitated Vehicle Routing Problem](examples/capacitatedvehiclerouting.hpp)
* Pricing problem: Elementary Shortest Path Problem with Resource Constraint [solved by Heuristic Tree Search](examples/pricingsolver/espprc.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Vehicle Routing Problem with Time Windows](examples/vehicleroutingwithtimewindows.hpp)
* Pricing problem: Elementary Shortest Path Problem with Resource Constraint and Time Windows [solved by Heuristic Tree Search](examples/pricingsolver/espprctw.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

<details><summary>Benchmarks</summary>
<p>

```shell
DATE=$(date '+%Y-%m-%d--%H-%M-%S') && python3 ../optimizationtools/scripts/bench_run.py --main ./bazel-bin/examples/vehicleroutingwithtimewindows_main --csv ../ordata/vehicleroutingwithtimewindows/data.csv -f "row['Dataset'] == 'solomon1987'" -l "${DATE}_vehicleroutingwithtimewindows"" -a "limited_discrepancy_search" -t 120
python3 ../optimizationtools/scripts/bench_process.py --csv ../ordata/vehicleroutingwithtimewindows/data.csv -f "row['Dataset'] == 'solomon1987'" -l "${DATE}_vehicleroutingwithtimewindows" -b heuristiclong -t 62
```

</p>
</details>

### Scheduling

Star observation scheduling problem from [fontanf/starobservationscheduling](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/algorithms/column_generation.hpp)
* Pricing problem: single-night star observation scheduling problem [solved by dynamic programming](https://github.com/fontanf/starobservationschedulingsolver/blob/main/singlenightstarobservationschedulingsolver/algorithms/dynamic_programming.hpp)

### Graphs

Graph Coloring Problem from [fontanf/coloringsolver](https://github.com/fontanf/coloringsolver/blob/master/coloringsolver/algorithms/columngeneration.cpp)
* Pricing problem: Maximum-Weight Independent Set Problem solved with the `localsearch` algorithm from [fontanf/stablesolver](https://github.com/fontanf/stablesolver) implemented with [fontanf/localsearchsolver](https://github.com/fontanf/localsearchsolver)

## Usage, running examples from command line

You need to have a linear programming solver already installed. Then update the corresponding entry in the `WORKSPACE` file. You may only need to update the `path` attribute of the solver you are using. Then, compile with one of the following command:
```shell
bazel build --define clp=true -- //...
bazel build --define cplex=true -- //...
```

Examples:

```shell
./bazel-bin/examples/cuttingstock_main -v 1 -a "column_generation" -i "../ordata/cuttingstock/falkenauer1996/T/Falkenauer_t120_00.txt"
```
```
Instance
--------
Number of item_types:  120
Capacity:              1000

======================================
       Column Generation Solver       
======================================

Algorithm
---------
Column Generation

Parameters
----------
Linear programming solver:               CLP
Static Wentges smoothing parameter:      0
Static directional smoothing parameter:  0
Self-adjusting Wentges smoothing:        0
Automatic directional smoothing:         0
Maximum number of iterations:            -1

      Time        It                 Obj       Col
      ----        --                 ---       ---
     0.004         1         240.0000000         0
     0.005         2         235.0000000         1
     0.005         3         235.0000000         2
     0.006         4         235.0000000         3
     0.007         5         235.0000000         4
     0.008         6         233.3333333         5
     0.009         7         228.3333333         6
     0.010         8         228.3333333         7
     0.011         9         228.3333333         8
     0.011        10         228.3333333         9
     0.012        11         226.6666667        10
     0.013        12         221.6666667        11
     0.014        13         221.6666667        12
     0.015        14         221.6666667        13
     0.016        15         221.6666667        14
     0.016        16         220.0000000        15
     0.017        17         215.0000000        16
     0.018        18         215.0000000        17
     0.018        19         215.0000000        18
     0.019        20         215.0000000        19
     0.020        21         213.3333333        20
     0.020        22         208.3333333        21
     0.021        23         208.3333333        22
     0.021        24         208.3333333        23
     0.022        25         208.3333333        24
     0.023        26         206.6666667        25
     0.023        27         201.6666667        26
     0.023        28         201.6666667        27
     0.024        29         201.6666667        28
     0.024        30         201.6666667        29
     0.025        31         200.0000000        30
     0.025        32         195.0000000        31
     0.026        33         195.0000000        32
     0.026        34         195.0000000        33
     0.027        35         195.0000000        34
     0.027        36         193.3333333        35
     0.027        37         188.3333333        36
     0.028        38         188.3333333        37
     0.028        39         188.3333333        38
     0.028        40         188.3333333        39
     0.029        41         186.6666667        40
     0.029        42         181.6666667        41
     0.029        43         181.6666667        42
     0.030        44         181.6666667        43
     0.030        45         181.6666667        44
     0.031        46         180.0000000        45
     0.031        47         175.0000000        46
     0.031        48         175.0000000        47
     0.032        49         175.0000000        48
     0.032        50         175.0000000        49
     0.032        51         173.3333333        50
     0.033        52         168.3333333        51
     0.033        53         168.3333333        52
     0.033        54         168.3333333        53
     0.034        55         168.3333333        54
     0.034        56         166.6666667        55
     0.034        57         161.6666667        56
     0.034        58         161.6666667        57
     0.035        59         161.6666667        58
     0.035        60         161.6666667        59
     0.035        61         160.0000000        60
     0.036        62         155.0000000        61
     0.036        63         155.0000000        62
     0.036        64         155.0000000        63
     0.036        65         155.0000000        64
     0.037        66         153.3333333        65
     0.037        67         148.3333333        66
     0.037        68         148.3333333        67
     0.038        69         148.3333333        68
     0.038        70         148.3333333        69
     0.038        71         146.6666667        70
     0.038        72         141.6666667        71
     0.038        73         141.6666667        72
     0.039        74         141.6666667        73
     0.039        75         141.6666667        74
     0.039        76         140.0000000        75
     0.039        77         135.0000000        76
     0.040        78         135.0000000        77
     0.040        79         135.0000000        78
     0.040        80         135.0000000        79
     0.040        81         133.3333333        80
     0.041        82         128.3333333        81
     0.041        83         128.3333333        82
     0.041        84         128.3333333        83
     0.041        85         128.3333333        84
     0.042        86         126.6666667        85
     0.042        87         123.3333333        86
     0.042        88         123.3333333        87
     0.042        89         123.3333333        88
     0.043        90         123.3333333        89
     0.043        91         123.3333333        90
     0.043        92         123.3333333        91
     0.043        93         123.3333333        92
     0.044        94         121.6666667        93
     0.044        95         120.8333333        94
     0.044        96         120.8333333        95
     0.044        97         120.0000000        96
     0.045        98         117.0833333        97
     0.045        99         116.8518519        98
     0.045       100         115.8333333        99
     0.045       101         113.7500000       100
     0.046       102         113.6363636       101
     0.046       103         113.3333333       102
     0.046       104         111.0000000       103
     0.046       105         110.3571429       104
     0.047       106         110.2325581       105
     0.047       107         110.0892857       106
     0.047       108         109.5070423       107
     0.047       109         107.0192308       108
     0.048       110         106.2962963       109
     0.048       111         103.6363636       110
     0.048       112         102.9629630       111
     0.048       113         102.5000000       112
     0.049       114         101.0000000       113
     0.049       115         100.0000000       114
     0.049       116          99.3678161       115
     0.049       117          98.0952381       116
     0.050       118          97.4314574       117
     0.050       119          97.0000000       118
     0.050       120          96.7380952       119
     0.050       121          96.6934046       120
     0.051       122          96.6666667       121
     0.051       123          96.5819209       122
     0.051       124          94.1666667       123
     0.051       125          93.3333333       124
     0.052       126          92.6197318       125
     0.052       127          91.9934641       126
     0.052       128          90.8690221       127
     0.052       129          90.3633880       128
     0.053       130          90.0000000       129
     0.053       131          90.0000000       130
     0.053       132          90.0000000       131
     0.053       133          88.4108527       132
     0.054       134          88.3333333       133
     0.054       135          85.3333333       134
     0.054       136          85.3333333       135
     0.054       137          85.3333333       136
     0.055       138          83.8333333       137
     0.055       139          80.8333333       138
     0.055       140          80.8333333       139
     0.055       141          80.8333333       140
     0.055       142          79.3333333       141
     0.056       143          76.3333333       142
     0.056       144          76.3333333       143
     0.056       145          76.3333333       144
     0.056       146          74.8333333       145
     0.057       147          71.8333333       146
     0.057       148          71.8333333       147
     0.057       149          71.8333333       148
     0.057       150          70.3333333       149
     0.057       151          67.3333333       150
     0.058       152          67.3333333       151
     0.058       153          67.3333333       152
     0.058       154          65.8333333       153
     0.058       155          62.8333333       154
     0.059       156          62.8333333       155
     0.059       157          62.8333333       156
     0.059       158          61.3333333       157
     0.059       159          58.3333333       158
     0.059       160          58.3333333       159
     0.060       161          58.3333333       160
     0.060       162          56.8333333       161
     0.060       163          53.8333333       162
     0.060       164          53.8333333       163
     0.061       165          53.8333333       164
     0.061       166          52.3333333       165
     0.061       167          49.3333333       166
     0.061       168          49.3333333       167
     0.061       169          49.3333333       168
     0.062       170          47.8333333       169
     0.062       171          44.8333333       170
     0.062       172          44.8333333       171
     0.062       173          44.8333333       172
     0.063       174          44.7988341       173
     0.063       175          44.6688771       174
     0.063       176          44.6666667       175
     0.063       177          44.6458333       176
     0.064       178          44.5416667       177
     0.064       179          44.3250000       178
     0.064       180          44.3005051       179
     0.065       181          44.2916667       180
     0.065       182          44.1666667       181
     0.065       183          44.1562500       182
     0.065       184          44.1338384       183
     0.066       185          44.1250000       184
     0.066       186          44.0250000       185
     0.066       187          43.9198718       186
     0.067       188          43.8611111       187
     0.067       189          43.8453984       188
     0.067       190          43.8154762       189
     0.067       191          43.7916667       190
     0.068       192          43.7777778       191
     0.068       193          43.6845238       192
     0.068       194          43.6338384       193
     0.069       195          43.6338384       194
     0.069       196          43.5654762       195
     0.069       197          43.5468750       196
     0.070       198          43.5163502       197
     0.070       199          43.4821429       198
     0.070       200          43.4642857       199
     0.070       201          43.4583333       200
     0.071       202          43.4166667       201
     0.071       203          43.3137255       202
     0.071       204          43.2916667       203
     0.072       205          43.2171717       204
     0.072       206          43.2013889       205
     0.072       207          43.1916667       206
     0.073       208          43.1527778       207
     0.073       209          43.1416667       208
     0.073       210          43.1338384       209
     0.074       211          43.1250000       210
     0.074       212          43.0625000       211
     0.074       213          43.0000000       212
     0.075       214          43.0000000       213
     0.075       215          42.9704301       214
     0.075       216          42.9166667       215
     0.076       217          42.9166667       216
     0.076       218          42.8906582       217
     0.076       219          42.8021462       218
     0.077       220          42.7916667       219
     0.077       221          42.7491135       220
     0.078       222          42.6948485       221
     0.078       223          42.6666667       222
     0.078       224          42.6250000       223
     0.078       225          42.6250000       224
     0.079       226          42.5952381       225
     0.079       227          42.5649024       226
     0.080       228          42.4994929       227
     0.080       229          42.4583333       228
     0.080       230          42.4583333       229
     0.081       231          42.4277025       230
     0.081       232          42.3596251       231
     0.081       233          42.3471074       232
     0.082       234          42.3085536       233
     0.082       235          42.2484022       234
     0.083       236          42.2250000       235
     0.083       237          42.1598968       236
     0.083       238          42.1440115       237
     0.084       239          42.1416898       238
     0.084       240          42.0000000       239
     0.085       241          41.9640719       240
     0.085       242          41.9287611       241
     0.085       243          41.8847403       242
     0.086       244          41.8750000       243
     0.086       245          41.8041667       244
     0.087       246          41.7973557       245
     0.087       247          41.7916667       246
     0.087       248          41.7161600       247
     0.088       249          41.6571311       248
     0.088       250          41.6557987       249
     0.089       251          41.6297609       250
     0.089       252          41.6250000       251
     0.089       253          41.6250000       252
     0.090       254          41.6250000       253
     0.090       255          41.5366755       254
     0.091       256          41.5277778       255
     0.091       257          41.5235100       256
     0.091       258          41.5169082       257
     0.092       259          41.5169082       258
     0.092       260          41.4741326       259
     0.093       261          41.4583333       260
     0.093       262          41.4233493       261
     0.094       263          41.3925867       262
     0.094       264          41.3678511       263
     0.094       265          41.3196865       264
     0.095       266          41.2916667       265
     0.095       267          41.2847347       266
     0.096       268          41.1503298       267
     0.096       269          41.1297105       268
     0.097       270          41.1250000       269
     0.097       271          41.1250000       270
     0.097       272          41.1250000       271
     0.097       273          41.1250000       272
     0.098       274          41.1250000       273
     0.098       275          41.0999117       274
     0.099       276          41.0748198       275
     0.099       277          41.0574656       276
     0.100       278          41.0550731       277
     0.100       279          41.0120256       278
     0.101       280          40.9628623       279
     0.101       281          40.9105874       280
     0.102       282          40.8567770       281
     0.102       283          40.8378257       282
     0.103       284          40.8226432       283
     0.104       285          40.7931120       284
     0.104       286          40.7916667       285
     0.104       287          40.7916667       286
     0.105       288          40.7916667       287
     0.105       289          40.7916667       288
     0.105       290          40.7916667       289
     0.106       291          40.7832796       290
     0.106       292          40.7782823       291
     0.107       293          40.7572986       292
     0.107       294          40.7457561       293
     0.108       295          40.7325257       294
     0.108       296          40.7234991       295
     0.109       297          40.6876642       296
     0.109       298          40.6390789       297
     0.110       299          40.6250000       298
     0.110       300          40.6250000       299
     0.111       301          40.5868311       300
     0.111       302          40.5712617       301
     0.112       303          40.5597551       302
     0.112       304          40.5568231       303
     0.113       305          40.5538371       304
     0.113       306          40.5377411       305
     0.114       307          40.5351025       306
     0.114       308          40.5259855       307
     0.114       309          40.5227664       308
     0.115       310          40.5096555       309
     0.115       311          40.4933308       310
     0.116       312          40.4769895       311
     0.117       313          40.4683552       312
     0.117       314          40.4564895       313
     0.118       315          40.4408915       314
     0.118       316          40.4407872       315
     0.119       317          40.4375949       316
     0.119       318          40.4310696       317
     0.120       319          40.4266260       318
     0.120       320          40.4228594       319
     0.121       321          40.4218089       320
     0.121       322          40.4063249       321
     0.122       323          40.4018445       322
     0.122       324          40.3860170       323
     0.123       325          40.3822757       324
     0.123       326          40.3804199       325
     0.124       327          40.3732700       326
     0.124       328          40.3732700       327
     0.125       329          40.3595053       328
     0.125       330          40.3475922       329
     0.126       331          40.3340996       330
     0.127       332          40.3238865       331
     0.127       333          40.3202829       332
     0.128       334          40.3156856       333
     0.128       335          40.3062468       334
     0.129       336          40.2971763       335
     0.129       337          40.2899217       336
     0.130       338          40.2847519       337
     0.131       339          40.2796066       338
     0.131       340          40.2611417       339
     0.132       341          40.2513228       340
     0.132       342          40.2439848       341
     0.133       343          40.2232747       342
     0.133       344          40.2179364       343
     0.134       345          40.2105393       344
     0.134       346          40.2057676       345
     0.135       347          40.2022580       346
     0.135       348          40.1997447       347
     0.136       349          40.1951389       348
     0.136       350          40.1901784       349
     0.137       351          40.1772124       350
     0.137       352          40.1763484       351
     0.138       353          40.1762352       352
     0.138       354          40.1723376       353
     0.139       355          40.1686207       354
     0.139       356          40.1683181       355
     0.140       357          40.1644082       356
     0.140       358          40.1596508       357
     0.141       359          40.1574335       358
     0.142       360          40.1468770       359
     0.142       361          40.1422333       360
     0.143       362          40.1398588       361
     0.143       363          40.1323958       362
     0.144       364          40.1237332       363
     0.144       365          40.1191313       364
     0.145       366          40.1147746       365
     0.145       367          40.1145649       366
     0.146       368          40.1120701       367
     0.146       369          40.1070167       368
     0.147       370          40.1065124       369
     0.147       371          40.1058458       370
     0.148       372          40.1058458       371
     0.148       373          40.1058458       372
     0.149       374          40.1051556       373
     0.149       375          40.1036858       374
     0.149       376          40.1030373       375
     0.150       377          40.1021049       376
     0.150       378          40.1020722       377
     0.151       379          40.0999253       378
     0.151       380          40.0975410       379
     0.152       381          40.0952180       380
     0.153       382          40.0932888       381
     0.153       383          40.0931392       382
     0.154       384          40.0919856       383
     0.154       385          40.0889257       384
     0.155       386          40.0870636       385
     0.156       387          40.0842193       386
     0.156       388          40.0812770       387
     0.157       389          40.0812625       388
     0.157       390          40.0808915       389
     0.158       391          40.0790988       390
     0.158       392          40.0774658       391
     0.159       393          40.0771196       392
     0.159       394          40.0761039       393
     0.160       395          40.0737319       394
     0.160       396          40.0721095       395
     0.161       397          40.0717586       396
     0.161       398          40.0701902       397
     0.162       399          40.0674845       398
     0.162       400          40.0669583       399
     0.163       401          40.0657453       400
     0.164       402          40.0637689       401
     0.164       403          40.0632397       402
     0.165       404          40.0628684       403
     0.166       405          40.0623881       404
     0.166       406          40.0598200       405
     0.167       407          40.0573759       406
     0.168       408          40.0559791       407
     0.168       409          40.0553166       408
     0.169       410          40.0543276       409
     0.169       411          40.0538753       410
     0.170       412          40.0528410       411
     0.171       413          40.0525252       412
     0.171       414          40.0519757       413
     0.172       415          40.0515598       414
     0.172       416          40.0511063       415
     0.173       417          40.0506880       416
     0.174       418          40.0482943       417
     0.175       419          40.0442655       418
     0.175       420          40.0410950       419
     0.176       421          40.0386172       420
     0.177       422          40.0370369       421
     0.177       423          40.0342659       422
     0.178       424          40.0335767       423
     0.178       425          40.0330474       424
     0.179       426          40.0324985       425
     0.179       427          40.0321209       426
     0.180       428          40.0320110       427
     0.181       429          40.0307773       428
     0.181       430          40.0293391       429
     0.182       431          40.0285761       430
     0.182       432          40.0280019       431
     0.183       433          40.0269699       432
     0.183       434          40.0254582       433
     0.184       435          40.0241844       434
     0.185       436          40.0241444       435
     0.185       437          40.0231451       436
     0.186       438          40.0213802       437
     0.187       439          40.0208737       438
     0.187       440          40.0205127       439
     0.188       441          40.0199062       440
     0.188       442          40.0194683       441
     0.189       443          40.0190988       442
     0.189       444          40.0189134       443
     0.190       445          40.0185413       444
     0.190       446          40.0178041       445
     0.191       447          40.0170922       446
     0.192       448          40.0166731       447
     0.193       449          40.0155447       448
     0.193       450          40.0152463       449
     0.194       451          40.0147887       450
     0.195       452          40.0144714       451
     0.195       453          40.0135606       452
     0.196       454          40.0131562       453
     0.197       455          40.0129607       454
     0.197       456          40.0119319       455
     0.198       457          40.0115344       456
     0.199       458          40.0113842       457
     0.199       459          40.0108683       458
     0.200       460          40.0108046       459
     0.200       461          40.0105718       460
     0.201       462          40.0104840       461
     0.202       463          40.0100546       462
     0.203       464          40.0098023       463
     0.203       465          40.0087032       464
     0.204       466          40.0084587       465
     0.205       467          40.0081064       466
     0.205       468          40.0078114       467
     0.206       469          40.0076686       468
     0.206       470          40.0075295       469
     0.207       471          40.0073042       470
     0.208       472          40.0072416       471
     0.209       473          40.0071786       472
     0.209       474          40.0068878       473
     0.210       475          40.0068263       474
     0.211       476          40.0062766       475
     0.212       477          40.0060930       476
     0.212       478          40.0059310       477
     0.213       479          40.0057052       478
     0.214       480          40.0051503       479
     0.215       481          40.0051358       480
     0.215       482          40.0047366       481
     0.216       483          40.0045146       482
     0.217       484          40.0039716       483
     0.218       485          40.0038181       484
     0.219       486          40.0037503       485
     0.219       487          40.0033462       486
     0.220       488          40.0030964       487
     0.221       489          40.0026741       488
     0.222       490          40.0023518       489
     0.222       491          40.0022530       490
     0.223       492          40.0020372       491
     0.224       493          40.0018547       492
     0.225       494          40.0016002       493
     0.226       495          40.0012965       494
     0.227       496          40.0012279       495
     0.227       497          40.0011992       496
     0.228       498          40.0009281       497
     0.229       499          40.0007958       498
     0.230       500          40.0007426       499
     0.231       501          40.0007161       500
     0.231       502          40.0007073       501
     0.232       503          40.0006298       502
     0.233       504          40.0005351       503
     0.234       505          40.0004966       504
     0.234       506          40.0003827       505
     0.235       507          40.0002929       506
     0.236       508          40.0000555       507
     0.237       509          40.0000000       508

Final statistics
----------------
Solution:                         40
Number of iterations:             509
Total number of columns:          508
Number of columns added:          508
Number of pricing:                509
Number of 1st try pricing:        509
Number of mispricing:             0
Number of pricing without stab.:  509
Time LP solve (s):                0.1721125
Time pricing (s):                 0.05419005
Total time (s):                   0.2371
```

```shell
./bazel-bin/examples/multipleknapsack_main -v 1 -a "limited_discrepancy_search" -i "../ordata/multipleknapsack/fukunaga2011/FK_1/random10_100_4_1000_1_1.txt"
```
```
Instance
--------
Number of knapsacks:  10
Number of items:      100

======================================
       Column Generation Solver       
======================================

Algorithm
---------
Limited Discrepancy Search

Parameters
----------
Linear programming solver:               CLP
Discrepancy limit:                       inf
Static Wentges smoothing parameter:      0
Static directional smoothing parameter:  0
Self-adjusting Wentges smoothing:        0
Automatic directional smoothing:         0

      Time      Solution         Bound           Gap   Gap (%)                 Comment
      ----      --------         -----           ---   -------                 -------
     0.033          -inf   26797.00000           inf      -nan               root node
     0.046   26797.00000   26797.00000       0.00000      0.00   node 10 discrepancy 0

Final statistics
----------------
Solution:                 2.7e+04
Bound:                    2.7e+04
Absolute gap:             1.1e-11
Relative gap (%):         4.1e-14
Total number of columns:  124
Number of columns added:  124
Total time (s):           0.046
Number of nodes:          11

Checker
-------
Number of items:                   49 / 100
Number of duplicates:              0
Number of overweighted knapsacks:  0
Feasible:                          1
Profit:                            26797
```

```shell
./bazel-bin/examples/capacitatedvehiclerouting_main --verbosity-level 1 --input ../ordata/capacitatedvehiclerouting/uchoa2014/X/X-n101-k25.vrp -a greedy -c sol.txt
```
```
Instance
--------
Number of locations:  101
Capacity:             206
Total demand:         5147
Demand ratio:         24.9854

======================================
       Column Generation Solver       
======================================

Algorithm
---------
Greedy

Parameters
----------
Linear programming solver:               CLP
Static Wentges smoothing parameter:      0
Static directional smoothing parameter:  0
Self-adjusting Wentges smoothing:        0
Automatic directional smoothing:         0

Final statistics
----------------
Solution:                 6663
Bound:                    6268.15
Absolute gap:             394.851
Relative gap (%):         5.92603
Total number of columns:  3711
Number of columns added:  3711
Total time (s):           4.72573

Checker
-------
Number of visited locations:    100 / 100
Number of duplicates:           0
Number of routes:               26
Number of overloaded vehicles:  0
Feasible:                       1
Total distance:                 6663
```

## Usage, C++ library

See examples.

