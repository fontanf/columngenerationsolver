# ColumnGenerationSolver

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

When the sub-problems can be solved with a very efficient algorithm - typically a pseudo-polynomial dynamic programming algorithm - then the bottleneck is the resolution of the linear problems. This is the case for the examples cutting stock, multiple knapsack, generalized assignment and star observation scheduling.

When the sub-problems are more difficult to solve, their resolution become the bottleneck of the algorithm. This is the case for the examples geometrical variable-sized bin packing, bin packing with conflicts, capacitated vehicle routing, vehicle routing problem with time windows and graph coloring. Here, these sub-problems are solved using generic approaches based on heuristic tree search or local search. During the first column generation iterations, these heuristic algorithms are stopped early to avoid spending a lot of time to find trivial columns.

### Packing

[Cutting stock problem](examples/cuttingstock.hpp)
* Pricing problem: bounded knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

<details><summary>Benchmarks</summary>
<p>

* Benchmarks:
  * `python3 ../optimizationtools/optimizationtools/bench_run.py --main ./bazel-bin/examples/cuttingstock_main --csv ../ordata/cuttingstock/data.csv -l cuttingstock -a "heuristic_tree_search" -t 60`

</p>
</details>

[Multiple knapsack problem](examples/multipleknapsack.hpp)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

<details><summary>Benchmarks</summary>
<p>

* Benchmarks:
  * `python3 ../optimizationtools/optimizationtools/bench_run.py --main ./bazel-bin/examples/multipleknapsack_main --csv ../ordata/multipleknapsack/data.csv -l multipleknapsack -a "heuristic_tree_search" -t 10`

</p>
</details>

[Generalized assignment problem](https://github.com/fontanf/generalizedassignmentsolver/blob/master/generalizedassignmentsolver/algorithms/columngeneration.cpp) from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Geometrical cutting stock, variable-sized bin packing and multiple knapsack problems](https://github.com/fontanf/packingsolver/blob/master/packingsolver/algorithms/column_generation.cpp) from [fontanf/packingsolver](https://github.com/fontanf/packingsolver)
* Pricing problem: geometrical knapsack problems solved with the algorithms from the same repository

[Bin packing problem with conflicts](examples/binpackingwithconflicts.hpp)
* Pricing problem: knapsack problem with conflicts solved with the [heuristic tree search](https://github.com/fontanf/treesearchsolver/blob/main/examples/knapsackwithconflicts.hpp) algorithm from [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Routing

[Capacitated vehicle routing problem](examples/capacitatedvehiclerouting.hpp)
* Pricing problem: elementary shortest path problem with resource constraint [solved by heuristic tree search](examples/pricingsolver/espprc.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Vehicle routing problem with time windows](examples/vehicleroutingwithtimewindows.hpp)
* Pricing problem: elementary shortest path problem with resource constraint and time windows [solved by heuristic tree search](examples/pricingsolver/espprctw.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

<details><summary>Benchmarks</summary>
<p>

```shell
DATE=$(date '+%Y-%m-%d--%H-%M-%S') && python3 ../optimizationtools/scripts/bench_run.py --main ./bazel-bin/examples/vehicleroutingwithtimewindows_main --csv ../ordata/vehicleroutingwithtimewindows/data.csv -f "row['Dataset'] == 'solomon1987'" -l "${DATE}_vehicleroutingwithtimewindows"" -a "limited_discrepancy_search" -t 120
python3 ../optimizationtools/scripts/bench_process.py --csv ../ordata/vehicleroutingwithtimewindows/data.csv -f "row['Dataset'] == 'solomon1987'" -l "${DATE}_vehicleroutingwithtimewindows" -b heuristiclong -t 62
```

</p>
</details>

### Scheduling

[Star observation scheduling problem](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/algorithms/column_generation.cpp) and [flexible star observation scheduling problem](https://github.com/fontanf/starobservationschedulingsolver/blob/main/flexiblestarobservationschedulingsolver/algorithms/column_generation.cpp) from [fontanf/starobservationscheduling](https://github.com/fontanf/starobservationschedulingsolver)
* Pricing problem: single-night star observation scheduling problem [solved by dynamic programming](https://github.com/fontanf/starobservationschedulingsolver/blob/main/singlenightstarobservationschedulingsolver/algorithms/dynamic_programming.hpp)

### Graphs

[Graph coloring problem](https://github.com/fontanf/coloringsolver/blob/master/coloringsolver/algorithms/columngeneration.cpp) from [fontanf/coloringsolver](https://github.com/fontanf/coloringsolver)
* Pricing problem: maximum-weight independent set problem solved with the `localsearch` algorithm from [fontanf/stablesolver](https://github.com/fontanf/stablesolver) implemented with [fontanf/localsearchsolver](https://github.com/fontanf/localsearchsolver)

## Usage, running examples from command line

You need to have a linear programming solver already installed. Then update the corresponding entry in the `WORKSPACE` file. You may only need to update the `path` attribute of the solver you are using. Then, compile with one of the following command:
```shell
bazel build --define clp=true -- //...
bazel build --define cplex=true -- //...
```

Examples:

```shell
./bazel-bin/examples/cuttingstock_main -v 1 -a "column-generation" -i "../ordata/cuttingstock/falkenauer1996/T/Falkenauer_t120_00.txt"
```
```
Instance
--------
Number of item_types:  120
Capacity:              1000

======================================
        ColumnGenerationSolver        
======================================

Algorithm
---------
Column generation

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
     0.011         1         240.0000000         0
     0.012         2         235.0000000         1
     0.013         3         235.0000000         2
     0.014         4         235.0000000         3
     0.015         5         235.0000000         4
     0.015         6         233.3333333         5
     0.016         7         228.3333333         6
     0.017         8         228.3333333         7
     0.018         9         228.3333333         8
     0.019        10         228.3333333         9
     0.019        11         226.6666667        10
     0.020        12         221.6666667        11
     0.021        13         221.6666667        12
     0.022        14         221.6666667        13
     0.023        15         221.6666667        14
     0.023        16         220.0000000        15
     0.024        17         215.0000000        16
     0.025        18         215.0000000        17
     0.026        19         215.0000000        18
     0.027        20         215.0000000        19
     0.028        21         213.3333333        20
     0.028        22         208.3333333        21
     0.029        23         208.3333333        22
     0.030        24         208.3333333        23
     0.031        25         208.3333333        24
     0.031        26         206.6666667        25
     0.032        27         201.6666667        26
     0.033        28         201.6666667        27
     0.033        29         201.6666667        28
     0.034        30         201.6666667        29
     0.034        31         200.0000000        30
     0.035        32         195.0000000        31
     0.035        33         195.0000000        32
     0.036        34         195.0000000        33
     0.036        35         195.0000000        34
     0.037        36         193.3333333        35
     0.037        37         188.3333333        36
     0.038        38         188.3333333        37
     0.038        39         188.3333333        38
     0.038        40         188.3333333        39
     0.039        41         186.6666667        40
     0.039        42         181.6666667        41
     0.040        43         181.6666667        42
     0.040        44         181.6666667        43
     0.040        45         181.6666667        44
     0.041        46         180.0000000        45
     0.041        47         175.0000000        46
     0.041        48         175.0000000        47
     0.042        49         175.0000000        48
     0.042        50         175.0000000        49
     0.042        51         173.3333333        50
     0.043        52         168.3333333        51
     0.043        53         168.3333333        52
     0.043        54         168.3333333        53
     0.044        55         168.3333333        54
     0.044        56         166.6666667        55
     0.044        57         161.6666667        56
     0.045        58         161.6666667        57
     0.045        59         161.6666667        58
     0.045        60         161.6666667        59
     0.046        61         160.0000000        60
     0.046        62         155.0000000        61
     0.046        63         155.0000000        62
     0.047        64         155.0000000        63
     0.047        65         155.0000000        64
     0.047        66         153.3333333        65
     0.047        67         148.3333333        66
     0.048        68         148.3333333        67
     0.048        69         148.3333333        68
     0.048        70         148.3333333        69
     0.048        71         146.6666667        70
     0.049        72         141.6666667        71
     0.049        73         141.6666667        72
     0.049        74         141.6666667        73
     0.049        75         141.6666667        74
     0.050        76         140.0000000        75
     0.050        77         135.0000000        76
     0.050        78         135.0000000        77
     0.051        79         135.0000000        78
     0.051        80         135.0000000        79
     0.051        81         133.3333333        80
     0.051        82         128.3333333        81
     0.051        83         128.3333333        82
     0.052        84         128.3333333        83
     0.052        85         128.3333333        84
     0.052        86         126.6666667        85
     0.052        87         123.3333333        86
     0.053        88         123.3333333        87
     0.053        89         123.3333333        88
     0.053        90         123.3333333        89
     0.053        91         123.3333333        90
     0.054        92         123.3333333        91
     0.054        93         123.3333333        92
     0.054        94         121.6666667        93
     0.054        95         120.8333333        94
     0.055        96         120.8333333        95
     0.055        97         120.0000000        96
     0.055        98         117.0833333        97
     0.055        99         116.8518519        98
     0.056       100         115.8333333        99
     0.056       101         113.7500000       100
     0.056       102         113.6363636       101
     0.056       103         113.3333333       102
     0.057       104         111.0000000       103
     0.057       105         110.3571429       104
     0.057       106         110.2325581       105
     0.057       107         110.0892857       106
     0.057       108         109.5070423       107
     0.057       109         107.0192308       108
     0.058       110         106.2962963       109
     0.058       111         103.6363636       110
     0.058       112         102.9629630       111
     0.058       113         102.5000000       112
     0.059       114         101.0000000       113
     0.059       115         100.0000000       114
     0.059       116          99.3678161       115
     0.059       117          98.0952381       116
     0.059       118          97.4314574       117
     0.060       119          97.0000000       118
     0.060       120          96.7380952       119
     0.060       121          96.6934046       120
     0.060       122          96.6666667       121
     0.061       123          96.5819209       122
     0.061       124          94.1666667       123
     0.061       125          93.3333333       124
     0.061       126          92.6197318       125
     0.061       127          91.9934641       126
     0.062       128          90.8690221       127
     0.062       129          90.3633880       128
     0.062       130          90.0000000       129
     0.062       131          90.0000000       130
     0.063       132          90.0000000       131
     0.063       133          88.4108527       132
     0.063       134          88.3333333       133
     0.063       135          85.3333333       134
     0.063       136          85.3333333       135
     0.064       137          85.3333333       136
     0.064       138          83.8333333       137
     0.064       139          80.8333333       138
     0.064       140          80.8333333       139
     0.064       141          80.8333333       140
     0.065       142          79.3333333       141
     0.065       143          76.3333333       142
     0.065       144          76.3333333       143
     0.065       145          76.3333333       144
     0.065       146          74.8333333       145
     0.066       147          71.8333333       146
     0.066       148          71.8333333       147
     0.066       149          71.8333333       148
     0.066       150          70.3333333       149
     0.066       151          67.3333333       150
     0.067       152          67.3333333       151
     0.067       153          67.3333333       152
     0.067       154          65.8333333       153
     0.067       155          62.8333333       154
     0.067       156          62.8333333       155
     0.068       157          62.8333333       156
     0.068       158          61.3333333       157
     0.068       159          58.3333333       158
     0.068       160          58.3333333       159
     0.068       161          58.3333333       160
     0.069       162          56.8333333       161
     0.069       163          53.8333333       162
     0.069       164          53.8333333       163
     0.069       165          53.8333333       164
     0.069       166          52.3333333       165
     0.070       167          49.3333333       166
     0.070       168          49.3333333       167
     0.070       169          49.3333333       168
     0.070       170          47.8333333       169
     0.071       171          44.8333333       170
     0.071       172          44.8333333       171
     0.071       173          44.8333333       172
     0.071       174          44.7988341       173
     0.071       175          44.6688771       174
     0.072       176          44.6666667       175
     0.072       177          44.6458333       176
     0.072       178          44.5416667       177
     0.073       179          44.3250000       178
     0.073       180          44.3005051       179
     0.073       181          44.2916667       180
     0.073       182          44.1666667       181
     0.074       183          44.1562500       182
     0.074       184          44.1338384       183
     0.074       185          44.1250000       184
     0.074       186          44.0250000       185
     0.075       187          43.9198718       186
     0.075       188          43.8611111       187
     0.075       189          43.8453984       188
     0.076       190          43.8154762       189
     0.076       191          43.7916667       190
     0.076       192          43.7777778       191
     0.076       193          43.6845238       192
     0.077       194          43.6338384       193
     0.077       195          43.6338384       194
     0.077       196          43.5654762       195
     0.077       197          43.5468750       196
     0.078       198          43.5163502       197
     0.078       199          43.4821429       198
     0.078       200          43.4642857       199
     0.079       201          43.4583333       200
     0.079       202          43.4166667       201
     0.079       203          43.3137255       202
     0.080       204          43.2916667       203
     0.080       205          43.2171717       204
     0.080       206          43.2013889       205
     0.080       207          43.1916667       206
     0.081       208          43.1527778       207
     0.081       209          43.1416667       208
     0.081       210          43.1338384       209
     0.082       211          43.1250000       210
     0.082       212          43.0625000       211
     0.082       213          43.0000000       212
     0.083       214          43.0000000       213
     0.083       215          42.9704301       214
     0.083       216          42.9166667       215
     0.084       217          42.9166667       216
     0.084       218          42.8906582       217
     0.084       219          42.8021462       218
     0.085       220          42.7916667       219
     0.085       221          42.7491135       220
     0.085       222          42.6948485       221
     0.086       223          42.6666667       222
     0.086       224          42.6250000       223
     0.086       225          42.6250000       224
     0.087       226          42.5952381       225
     0.087       227          42.5649024       226
     0.087       228          42.4994929       227
     0.088       229          42.4583333       228
     0.088       230          42.4583333       229
     0.088       231          42.4277025       230
     0.089       232          42.3596251       231
     0.089       233          42.3471074       232
     0.089       234          42.3085536       233
     0.090       235          42.2484022       234
     0.090       236          42.2250000       235
     0.091       237          42.1598968       236
     0.091       238          42.1440115       237
     0.091       239          42.1416898       238
     0.092       240          42.0000000       239
     0.092       241          41.9640719       240
     0.092       242          41.9287611       241
     0.093       243          41.8847403       242
     0.093       244          41.8750000       243
     0.093       245          41.8041667       244
     0.094       246          41.7973557       245
     0.094       247          41.7916667       246
     0.095       248          41.7161600       247
     0.095       249          41.6571311       248
     0.095       250          41.6557987       249
     0.096       251          41.6297609       250
     0.096       252          41.6250000       251
     0.097       253          41.6250000       252
     0.097       254          41.6250000       253
     0.097       255          41.5366755       254
     0.098       256          41.5277778       255
     0.098       257          41.5235100       256
     0.098       258          41.5169082       257
     0.099       259          41.5169082       258
     0.099       260          41.4741326       259
     0.100       261          41.4583333       260
     0.100       262          41.4233493       261
     0.100       263          41.3925867       262
     0.101       264          41.3678511       263
     0.101       265          41.3196865       264
     0.102       266          41.2916667       265
     0.102       267          41.2847347       266
     0.102       268          41.1503298       267
     0.103       269          41.1297105       268
     0.103       270          41.1250000       269
     0.104       271          41.1250000       270
     0.104       272          41.1250000       271
     0.104       273          41.1250000       272
     0.104       274          41.1250000       273
     0.105       275          41.0999117       274
     0.105       276          41.0748198       275
     0.106       277          41.0574656       276
     0.106       278          41.0550731       277
     0.107       279          41.0120256       278
     0.107       280          40.9628623       279
     0.108       281          40.9105874       280
     0.108       282          40.8567770       281
     0.109       283          40.8378257       282
     0.109       284          40.8226432       283
     0.110       285          40.7931120       284
     0.110       286          40.7916667       285
     0.111       287          40.7916667       286
     0.111       288          40.7916667       287
     0.111       289          40.7916667       288
     0.111       290          40.7916667       289
     0.112       291          40.7832796       290
     0.112       292          40.7782823       291
     0.113       293          40.7572986       292
     0.113       294          40.7457561       293
     0.114       295          40.7325257       294
     0.114       296          40.7234991       295
     0.115       297          40.6876642       296
     0.115       298          40.6390789       297
     0.116       299          40.6250000       298
     0.116       300          40.6250000       299
     0.117       301          40.5868311       300
     0.117       302          40.5712617       301
     0.118       303          40.5597551       302
     0.118       304          40.5568231       303
     0.118       305          40.5538371       304
     0.119       306          40.5377411       305
     0.119       307          40.5351025       306
     0.120       308          40.5259855       307
     0.120       309          40.5227664       308
     0.121       310          40.5096555       309
     0.121       311          40.4933308       310
     0.122       312          40.4769895       311
     0.122       313          40.4683552       312
     0.123       314          40.4564895       313
     0.123       315          40.4408915       314
     0.124       316          40.4407872       315
     0.124       317          40.4375949       316
     0.125       318          40.4310696       317
     0.125       319          40.4266260       318
     0.126       320          40.4228594       319
     0.126       321          40.4218089       320
     0.127       322          40.4063249       321
     0.127       323          40.4018445       322
     0.128       324          40.3860170       323
     0.128       325          40.3822757       324
     0.129       326          40.3804199       325
     0.129       327          40.3732700       326
     0.130       328          40.3732700       327
     0.130       329          40.3595053       328
     0.131       330          40.3475922       329
     0.131       331          40.3340996       330
     0.132       332          40.3238865       331
     0.132       333          40.3202829       332
     0.133       334          40.3156856       333
     0.133       335          40.3062468       334
     0.134       336          40.2971763       335
     0.134       337          40.2899217       336
     0.135       338          40.2847519       337
     0.135       339          40.2796066       338
     0.136       340          40.2611417       339
     0.137       341          40.2513228       340
     0.137       342          40.2439848       341
     0.138       343          40.2232747       342
     0.138       344          40.2179364       343
     0.139       345          40.2105393       344
     0.139       346          40.2057676       345
     0.140       347          40.2022580       346
     0.140       348          40.1997447       347
     0.140       349          40.1951389       348
     0.141       350          40.1901784       349
     0.141       351          40.1772124       350
     0.142       352          40.1763484       351
     0.142       353          40.1762352       352
     0.143       354          40.1723376       353
     0.143       355          40.1686207       354
     0.144       356          40.1683181       355
     0.144       357          40.1644082       356
     0.145       358          40.1596508       357
     0.145       359          40.1574335       358
     0.146       360          40.1468770       359
     0.146       361          40.1422333       360
     0.147       362          40.1398588       361
     0.147       363          40.1323958       362
     0.148       364          40.1237332       363
     0.148       365          40.1191313       364
     0.149       366          40.1147746       365
     0.149       367          40.1145649       366
     0.150       368          40.1120701       367
     0.150       369          40.1070167       368
     0.151       370          40.1065124       369
     0.151       371          40.1058458       370
     0.152       372          40.1058458       371
     0.152       373          40.1058458       372
     0.152       374          40.1051556       373
     0.153       375          40.1036858       374
     0.153       376          40.1030373       375
     0.154       377          40.1021049       376
     0.154       378          40.1020722       377
     0.155       379          40.0999253       378
     0.155       380          40.0975410       379
     0.156       381          40.0952180       380
     0.156       382          40.0932888       381
     0.157       383          40.0931392       382
     0.158       384          40.0919856       383
     0.158       385          40.0889257       384
     0.159       386          40.0870636       385
     0.159       387          40.0842193       386
     0.160       388          40.0812770       387
     0.160       389          40.0812625       388
     0.161       390          40.0808915       389
     0.161       391          40.0790988       390
     0.162       392          40.0774658       391
     0.162       393          40.0771196       392
     0.162       394          40.0761039       393
     0.163       395          40.0737319       394
     0.163       396          40.0721095       395
     0.164       397          40.0717586       396
     0.164       398          40.0701902       397
     0.165       399          40.0674845       398
     0.165       400          40.0669583       399
     0.166       401          40.0657453       400
     0.167       402          40.0637689       401
     0.167       403          40.0632397       402
     0.168       404          40.0628684       403
     0.169       405          40.0623881       404
     0.169       406          40.0598200       405
     0.170       407          40.0573759       406
     0.171       408          40.0559791       407
     0.171       409          40.0553166       408
     0.172       410          40.0543276       409
     0.172       411          40.0538753       410
     0.173       412          40.0528410       411
     0.173       413          40.0525252       412
     0.174       414          40.0519757       413
     0.174       415          40.0515598       414
     0.175       416          40.0511063       415
     0.176       417          40.0506880       416
     0.176       418          40.0482943       417
     0.177       419          40.0442655       418
     0.178       420          40.0410950       419
     0.178       421          40.0386172       420
     0.179       422          40.0370369       421
     0.179       423          40.0342659       422
     0.180       424          40.0335767       423
     0.181       425          40.0330474       424
     0.181       426          40.0324985       425
     0.182       427          40.0321209       426
     0.182       428          40.0320110       427
     0.183       429          40.0307773       428
     0.183       430          40.0293391       429
     0.184       431          40.0285761       430
     0.184       432          40.0280019       431
     0.185       433          40.0269699       432
     0.185       434          40.0254582       433
     0.186       435          40.0241844       434
     0.187       436          40.0241444       435
     0.187       437          40.0231451       436
     0.188       438          40.0213802       437
     0.188       439          40.0208737       438
     0.189       440          40.0205127       439
     0.189       441          40.0199062       440
     0.190       442          40.0194683       441
     0.190       443          40.0190988       442
     0.191       444          40.0189134       443
     0.191       445          40.0185413       444
     0.192       446          40.0178041       445
     0.193       447          40.0170922       446
     0.193       448          40.0166731       447
     0.194       449          40.0155447       448
     0.195       450          40.0152463       449
     0.195       451          40.0147887       450
     0.196       452          40.0144714       451
     0.197       453          40.0135606       452
     0.197       454          40.0131562       453
     0.198       455          40.0129607       454
     0.198       456          40.0119319       455
     0.199       457          40.0115344       456
     0.200       458          40.0113842       457
     0.200       459          40.0108683       458
     0.201       460          40.0108046       459
     0.202       461          40.0105718       460
     0.202       462          40.0104840       461
     0.203       463          40.0100546       462
     0.204       464          40.0098023       463
     0.204       465          40.0087032       464
     0.205       466          40.0084587       465
     0.205       467          40.0081064       466
     0.206       468          40.0078114       467
     0.207       469          40.0076686       468
     0.207       470          40.0075295       469
     0.208       471          40.0073042       470
     0.209       472          40.0072416       471
     0.209       473          40.0071786       472
     0.210       474          40.0068878       473
     0.211       475          40.0068263       474
     0.211       476          40.0062766       475
     0.212       477          40.0060930       476
     0.213       478          40.0059310       477
     0.214       479          40.0057052       478
     0.214       480          40.0051503       479
     0.215       481          40.0051358       480
     0.216       482          40.0047366       481
     0.217       483          40.0045146       482
     0.217       484          40.0039716       483
     0.218       485          40.0038181       484
     0.219       486          40.0037503       485
     0.220       487          40.0033462       486
     0.220       488          40.0030964       487
     0.221       489          40.0026741       488
     0.222       490          40.0023518       489
     0.223       491          40.0022530       490
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
Time LP solve (s):                0.1679066
Time pricing (s):                 0.0507851
Total time (s):                   0.2371
```

```shell
./bazel-bin/examples/multipleknapsack_main -v 1 -a "greedy" -i "../ordata/multipleknapsack/fukunaga2011/FK_1/random10_100_4_1000_1_1.txt"
```
```
Instance
--------
Number of knapsacks:  10
Number of items:      100

======================================
        ColumnGenerationSolver        
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
Solution:                 26797
Bound:                    26797
Absolute gap:             1.09139e-11
Relative gap (%):         4.07282e-14
Total number of columns:  124
Number of columns added:  124
Total time (s):           0.0473271
```

```shell
./bazel-bin/examples/capacitatedvehiclerouting_main --verbosity-level 1 --input ../ordata/capacitatedvehiclerouting/uchoa2014/X/X-n101-k25.vrp -a "limited-discrepancy-search" -t 60 -c sol.txt
```
```
Instance
--------
Number of locations:  101
Capacity:             206
Total demand:         5147
Demand ratio:         24.9854

======================================
        ColumnGenerationSolver        
======================================

Algorithm
---------
Limited discrepancy search

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
     1.646           inf    6279.17633           inf      -nan               root node
     5.279    6779.00000    6279.17633     499.82367      7.37   node 26 discrepancy 0
     5.281    6613.00000    6279.17633     333.82367      5.05   node 30 discrepancy 1
     6.893    6604.00000    6279.17633     324.82367      4.92   node 73 discrepancy 1
     9.347    6600.00000    6279.17633     320.82367      4.86  node 134 discrepancy 1
    18.203    6546.00000    6279.17633     266.82367      4.08  node 283 discrepancy 1
    27.779    6532.00000    6279.17633     252.82367      3.87  node 396 discrepancy 2
    28.109    6478.00000    6279.17633     198.82367      3.07  node 504 discrepancy 2
    28.112    6477.00000    6279.17633     197.82367      3.05  node 508 discrepancy 2

Final statistics
----------------
Solution:                 6.5e+03
Bound:                    6.3e+03
Absolute gap:             2e+02
Relative gap (%):         3.1
Total number of columns:  14252
Number of columns added:  14252
Total time (s):           60
Number of nodes:          1652

Checker
-------
Number of visited locations:    100 / 100
Number of duplicates:           0
Number of routes:               26
Number of overloaded vehicles:  0
Feasible:                       1
Total distance:                 6477
```

## Usage, C++ library

See examples.

