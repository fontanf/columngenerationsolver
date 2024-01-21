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
  * Column generation `column-generation`
  * Greedy `greedy`
  * Limited discrepancy search `limited-discrepancy-search`
  * Heuristic tree search `heuristic-tree-search`
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

[Multiple knapsack problem](examples/multipleknapsack.hpp)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Generalized assignment problem](https://github.com/fontanf/generalizedassignmentsolver/blob/master/generalizedassignmentsolver/algorithms/columngeneration.cpp) from [fontanf/generalizedassignmentsolver](https://github.com/fontanf/generalizedassignmentsolver)
* Pricing problem: knapsck problem solved with the `minknap` algorithm from [fontanf/knapsacksolver](https://github.com/fontanf/knapsacksolver)

[Geometrical cutting stock, variable-sized bin packing and multiple knapsack problems](https://github.com/fontanf/packingsolver/blob/master/packingsolver/algorithms/column_generation.hpp) from [fontanf/packingsolver](https://github.com/fontanf/packingsolver)
* Pricing problem: geometrical knapsack problems solved with the algorithms from the same repository

[Bin packing problem with conflicts](examples/binpackingwithconflicts.hpp)
* Pricing problem: knapsack problem with conflicts solved with the [heuristic tree search](https://github.com/fontanf/treesearchsolver/blob/main/examples/knapsackwithconflicts.hpp) algorithm from [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Routing

[Capacitated vehicle routing problem](examples/capacitatedvehiclerouting.hpp)
* Pricing problem: elementary shortest path problem with resource constraint [solved by heuristic tree search](examples/pricingsolver/espprc.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

[Vehicle routing problem with time windows](examples/vehicleroutingwithtimewindows.hpp)
* Pricing problem: elementary shortest path problem with resource constraint and time windows [solved by heuristic tree search](examples/pricingsolver/espprctw.hpp) using [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver)

### Scheduling

[Star observation scheduling problem](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/starobservationscheduling/algorithms/column_generation.cpp) and [flexible star observation scheduling problem](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/flexiblestarobservationscheduling/algorithms/column_generation.cpp) from [fontanf/starobservationscheduling](https://github.com/fontanf/starobservationschedulingsolver)
* Pricing problem: single-night star observation scheduling problem [solved by dynamic programming](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/singlenightstarobservationscheduling/algorithms/dynamic_programming.hpp) and single-night flexible star observation scheduling problem [solved by dynamic programming](https://github.com/fontanf/starobservationschedulingsolver/blob/main/starobservationschedulingsolver/flexiblesinglenightstarobservationscheduling/algorithms/dynamic_programming.hpp)

### Graphs

[Graph coloring problem](https://github.com/fontanf/coloringsolver/blob/master/coloringsolver/algorithms/column_generation.cpp) from [fontanf/coloringsolver](https://github.com/fontanf/coloringsolver)
* Pricing problem: maximum-weight independent set problem solved with the `local-search` algorithm from [fontanf/stablesolver](https://github.com/fontanf/stablesolver) implemented with [fontanf/localsearchsolver](https://github.com/fontanf/localsearchsolver)

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
==========================================
          ColumnGenerationSolver          
==========================================

Model
-----
Number of constraints:               120
Column lower bound:                  0
Column upper bound:                  1
Dummy column objective coefficient:  1

Algorithm
---------
Column generation

Parameters
----------
Time limit:                              inf
Messages
    Verbosity level:                     1
    Standard output:                     1
    File path:                           
    # streams:                           0
Logger
    Has logger:                          0
    Standard error:                      0
    File path:                           
Linear programming solver:               CLP
Static Wentges smoothing parameter:      0
Static directional smoothing parameter:  0
Self-adjusting Wentges smoothing:        0
Automatic directional smoothing:         0
Maximum number of iterations:            -1

Column generation
-----------------

        Time   Iteration   # columns                   Value
        ----   ---------   ---------                   -----
       0.001           1           0                     240
       0.001           2           1                     235
       0.002           3           2                     235
       0.002           4           3                     235
       0.003           5           4                     235
       0.003           6           5                 233.333
       0.004           7           6                 228.333
       0.004           8           7                 228.333
       0.005           9           8                 228.333
       0.005          10           9                 228.333
       0.006          11          10                 226.667
       0.006          12          11                 221.667
       0.007          13          12                 221.667
       0.007          14          13                 221.667
       0.008          15          14                 221.667
       0.008          16          15                     220
       0.009          17          16                     215
       0.009          18          17                     215
       0.010          19          18                     215
       0.010          20          19                     215
       0.011          21          20                 213.333
       0.012          22          21                 208.333
       0.012          23          22                 208.333
       0.013          24          23                 208.333
       0.013          25          24                 208.333
       0.013          26          25                 206.667
       0.014          27          26                 201.667
       0.014          28          27                 201.667
       0.015          29          28                 201.667
       0.015          30          29                 201.667
       0.016          31          30                     200
       0.016          32          31                     195
       0.017          33          32                     195
       0.017          34          33                     195
       0.018          35          34                     195
       0.018          36          35                 193.333
       0.019          37          36                 188.333
       0.019          38          37                 188.333
       0.020          39          38                 188.333
       0.020          40          39                 188.333
       0.021          41          40                 186.667
       0.021          42          41                 181.667
       0.021          43          42                 181.667
       0.022          44          43                 181.667
       0.022          45          44                 181.667
       0.023          46          45                     180
       0.023          47          46                     175
       0.023          48          47                     175
       0.024          49          48                     175
       0.024          50          49                     175
       0.025          51          50                 173.333
       0.025          52          51                 168.333
       0.025          53          52                 168.333
       0.026          54          53                 168.333
       0.026          55          54                 168.333
       0.027          56          55                 166.667
       0.027          57          56                 161.667
       0.028          58          57                 161.667
       0.028          59          58                 161.667
       0.028          60          59                 161.667
       0.029          61          60                     160
       0.029          62          61                     155
       0.029          63          62                     155
       0.030          64          63                     155
       0.030          65          64                     155
       0.030          66          65                 153.333
       0.031          67          66                 148.333
       0.031          68          67                 148.333
       0.031          69          68                 148.333
       0.032          70          69                 148.333
       0.032          71          70                 146.667
       0.032          72          71                 141.667
       0.032          73          72                 141.667
       0.033          74          73                 141.667
       0.033          75          74                 141.667
       0.033          76          75                     140
       0.034          77          76                     135
       0.034          78          77                     135
       0.034          79          78                     135
       0.034          80          79                     135
       0.035          81          80                 133.333
       0.035          82          81                 128.333
       0.035          83          82                 128.333
       0.035          84          83                 128.333
       0.036          85          84                 128.333
       0.036          86          85                 126.667
       0.036          87          86                 123.333
       0.037          88          87                 123.333
       0.037          89          88                 123.333
       0.038          90          89                 121.667
       0.038          91          90                     120
       0.038          92          91                     120
       0.039          93          92                     120
       0.039          94          93                 118.333
       0.040          95          94                 118.333
       0.040          96          95                 116.667
       0.040          97          96                     115
       0.041          98          97                 113.333
       0.041          99          98                 113.333
       0.042         100          99                 111.667
       0.042         101         100                     110
       0.043         102         101                     110
       0.043         103         102                 108.333
       0.043         104         103                     105
       0.044         105         104                     105
       0.044         106         105                     105
       0.044         107         106                     105
       0.045         108         107                     105
       0.045         109         108                     105
       0.045         110         109                     105
       0.046         111         110                 103.333
       0.046         112         111                 101.667
       0.047         113         112                     100
       0.047         114         113                 98.3333
       0.047         115         114                 98.3333
       0.048         116         115                 96.6667
       0.048         117         116                 96.6667
       0.048         118         117                 96.6667
       0.049         119         118                 96.6667
       0.049         120         119                      95
       0.049         121         120                      95
       0.050         122         121                 94.1667
       0.050         123         122                 91.6667
       0.051         124         123                 90.8333
       0.051         125         124                      90
       0.052         126         125                      87
       0.052         127         126                      87
       0.053         128         127                      87
       0.053         129         128                    85.5
       0.053         130         129                    82.5
       0.054         131         130                    82.5
       0.054         132         131                    82.5
       0.054         133         132                      81
       0.055         134         133                      78
       0.055         135         134                      78
       0.055         136         135                      78
       0.056         137         136                    76.5
       0.056         138         137                    73.5
       0.056         139         138                    73.5
       0.057         140         139                    73.5
       0.057         141         140                      72
       0.057         142         141                      69
       0.058         143         142                      69
       0.058         144         143                      69
       0.058         145         144                    67.5
       0.058         146         145                    64.5
       0.059         147         146                    64.5
       0.059         148         147                    64.5
       0.059         149         148                      63
       0.060         150         149                      60
       0.060         151         150                      60
       0.060         152         151                      60
       0.060         153         152                    58.5
       0.061         154         153                    55.5
       0.061         155         154                    55.5
       0.061         156         155                    55.5
       0.061         157         156                      54
       0.062         158         157                      51
       0.062         159         158                      51
       0.062         160         159                      51
       0.062         161         160                    49.5
       0.063         162         161                    46.5
       0.063         163         162                    46.5
       0.063         164         163                    46.5
       0.063         165         164                      45
       0.064         166         165                 44.9167
       0.065         167         166                 44.8333
       0.065         168         167                 44.8333
       0.066         169         168                   44.75
       0.066         170         169                   44.75
       0.067         171         170                 44.6667
       0.067         172         171                 44.6667
       0.068         173         172                 44.6667
       0.068         174         173                    44.5
       0.069         175         174                 44.4583
       0.070         176         175                 44.4583
       0.070         177         176                 44.3333
       0.071         178         177                 44.3333
       0.071         179         178                   44.25
       0.072         180         179                 44.1111
       0.072         181         180                 44.0417
       0.073         182         181                      44
       0.073         183         182                 43.9928
       0.074         184         183                 43.9537
       0.075         185         184                 43.8929
       0.075         186         185                 43.8788
       0.076         187         186                 43.8333
       0.077         188         187                 43.8333
       0.077         189         188                 43.8333
       0.078         190         189                 43.7915
       0.078         191         190                 43.7407
       0.079         192         191                 43.6027
       0.079         193         192                 43.5973
       0.080         194         193                 43.5973
       0.080         195         194                  43.537
       0.081         196         195                 43.5125
       0.082         197         196                 43.5125
       0.082         198         197                    43.5
       0.083         199         198                 43.4848
       0.084         200         199                   43.47
       0.084         201         200                 43.4466
       0.085         202         201                 43.4275
       0.086         203         202                 43.3934
       0.086         204         203                 43.3333
       0.087         205         204                 43.3333
       0.088         206         205                 43.2847
       0.088         207         206                 43.2415
       0.089         208         207                 43.0164
       0.089         209         208                      43
       0.090         210         209                 42.9765
       0.091         211         210                 42.9533
       0.092         212         211                  42.911
       0.092         213         212                  42.891
       0.093         214         213                 42.8333
       0.094         215         214                 42.8333
       0.094         216         215                 42.8152
       0.095         217         216                 42.8068
       0.095         218         217                 42.7804
       0.096         219         218                 42.6853
       0.097         220         219                 42.6787
       0.097         221         220                 42.6552
       0.098         222         221                 42.6074
       0.099         223         222                 42.5764
       0.099         224         223                  42.564
       0.100         225         224                 42.5553
       0.101         226         225                 42.5126
       0.101         227         226                    42.5
       0.102         228         227                 42.4244
       0.103         229         228                 42.4092
       0.103         230         229                 42.3974
       0.104         231         230                 42.3956
       0.105         232         231                 42.3333
       0.105         233         232                 42.1751
       0.106         234         233                 42.0612
       0.107         235         234                      42
       0.107         236         235                      42
       0.108         237         236                 41.9876
       0.109         238         237                 41.9334
       0.110         239         238                 41.8862
       0.110         240         239                 41.8501
       0.111         241         240                 41.8481
       0.112         242         241                 41.8333
       0.112         243         242                 41.8333
       0.113         244         243                  41.708
       0.114         245         244                 41.6916
       0.115         246         245                 41.6109
       0.115         247         246                 41.6078
       0.116         248         247                 41.5952
       0.117         249         248                 41.5152
       0.118         250         249                    41.5
       0.118         251         250                    41.5
       0.119         252         251                 41.4161
       0.120         253         252                 41.3687
       0.121         254         253                 41.3606
       0.122         255         254                 41.3333
       0.122         256         255                 41.3333
       0.123         257         256                 41.2565
       0.124         258         257                  41.184
       0.124         259         258                  41.098
       0.125         260         259                 41.0094
       0.126         261         260                 41.0079
       0.127         262         261                      41
       0.128         263         262                 40.9991
       0.129         264         263                 40.9832
       0.129         265         264                 40.9094
       0.130         266         265                 40.8396
       0.131         267         266                 40.8333
       0.131         268         267                 40.8333
       0.132         269         268                 40.8029
       0.133         270         269                 40.7812
       0.134         271         270                 40.7732
       0.134         272         271                  40.761
       0.135         273         272                 40.7438
       0.136         274         273                 40.7438
       0.136         275         274                 40.7438
       0.137         276         275                 40.6878
       0.137         277         276                 40.6617
       0.138         278         277                 40.6294
       0.139         279         278                 40.5979
       0.140         280         279                  40.541
       0.140         281         280                 40.5237
       0.141         282         281                 40.5205
       0.142         283         282                 40.5072
       0.142         284         283                    40.5
       0.143         285         284                    40.5
       0.144         286         285                    40.5
       0.145         287         286                 40.4907
       0.146         288         287                 40.4647
       0.146         289         288                 40.4456
       0.147         290         289                  40.423
       0.148         291         290                 40.4136
       0.149         292         291                 40.3944
       0.150         293         292                 40.3777
       0.150         294         293                 40.3743
       0.151         295         294                 40.3741
       0.152         296         295                 40.3569
       0.153         297         296                 40.3389
       0.154         298         297                 40.3333
       0.155         299         298                 40.3333
       0.155         300         299                 40.3333
       0.156         301         300                 40.3333
       0.156         302         301                 40.3212
       0.158         303         302                 40.2965
       0.159         304         303                 40.2881
       0.160         305         304                 40.2724
       0.161         306         305                 40.2603
       0.161         307         306                 40.2508
       0.162         308         307                 40.2474
       0.163         309         308                 40.2262
       0.164         310         309                 40.2216
       0.165         311         310                 40.2206
       0.166         312         311                 40.2203
       0.168         313         312                 40.2115
       0.169         314         313                 40.1983
       0.170         315         314                 40.1973
       0.171         316         315                 40.1867
       0.172         317         316                 40.1733
       0.173         318         317                 40.1625
       0.174         319         318                 40.1592
       0.175         320         319                 40.1542
       0.176         321         320                 40.1513
       0.177         322         321                 40.1501
       0.178         323         322                 40.1455
       0.179         324         323                 40.1422
       0.180         325         324                 40.1397
       0.181         326         325                 40.1388
       0.182         327         326                 40.1388
       0.182         328         327                 40.1306
       0.183         329         328                   40.13
       0.184         330         329                 40.1249
       0.185         331         330                 40.1229
       0.185         332         331                 40.1196
       0.186         333         332                 40.1138
       0.187         334         333                 40.1138
       0.188         335         334                 40.1135
       0.190         336         335                 40.1111
       0.190         337         336                 40.1105
       0.191         338         337                 40.1092
       0.192         339         338                 40.1035
       0.193         340         339                 40.1015
       0.195         341         340                 40.0982
       0.196         342         341                 40.0952
       0.197         343         342                 40.0926
       0.199         344         343                 40.0906
       0.200         345         344                 40.0876
       0.201         346         345                 40.0875
       0.201         347         346                 40.0862
       0.202         348         347                  40.085
       0.203         349         348                 40.0834
       0.204         350         349                 40.0815
       0.205         351         350                 40.0795
       0.206         352         351                 40.0783
       0.206         353         352                  40.075
       0.207         354         353                 40.0717
       0.209         355         354                 40.0707
       0.210         356         355                 40.0689
       0.210         357         356                 40.0671
       0.212         358         357                 40.0648
       0.213         359         358                 40.0634
       0.214         360         359                 40.0617
       0.214         361         360                 40.0607
       0.216         362         361                 40.0595
       0.216         363         362                 40.0583
       0.217         364         363                 40.0559
       0.218         365         364                 40.0528
       0.219         366         365                 40.0511
       0.219         367         366                 40.0476
       0.220         368         367                 40.0466
       0.222         369         368                  40.045
       0.222         370         369                 40.0429
       0.223         371         370                 40.0426
       0.224         372         371                 40.0409
       0.225         373         372                 40.0387
       0.226         374         373                 40.0375
       0.227         375         374                 40.0356
       0.228         376         375                 40.0337
       0.229         377         376                 40.0329
       0.230         378         377                 40.0321
       0.231         379         378                 40.0315
       0.232         380         379                  40.031
       0.233         381         380                 40.0301
       0.234         382         381                 40.0295
       0.236         383         382                 40.0295
       0.237         384         383                 40.0281
       0.238         385         384                 40.0271
       0.239         386         385                 40.0259
       0.241         387         386                 40.0254
       0.242         388         387                 40.0249
       0.243         389         388                 40.0248
       0.243         390         389                 40.0239
       0.244         391         390                 40.0235
       0.245         392         391                 40.0228
       0.247         393         392                 40.0216
       0.247         394         393                  40.021
       0.248         395         394                 40.0203
       0.249         396         395                 40.0194
       0.250         397         396                 40.0185
       0.251         398         397                 40.0184
       0.252         399         398                 40.0176
       0.253         400         399                 40.0175
       0.254         401         400                 40.0174
       0.255         402         401                 40.0171
       0.256         403         402                 40.0167
       0.257         404         403                 40.0164
       0.258         405         404                 40.0161
       0.259         406         405                 40.0158
       0.260         407         406                 40.0153
       0.261         408         407                  40.015
       0.262         409         408                 40.0145
       0.263         410         409                 40.0138
       0.264         411         410                 40.0132
       0.265         412         411                 40.0128
       0.266         413         412                 40.0122
       0.266         414         413                  40.012
       0.268         415         414                 40.0118
       0.269         416         415                 40.0116
       0.270         417         416                 40.0114
       0.270         418         417                 40.0107
       0.272         419         418                 40.0099
       0.273         420         419                 40.0099
       0.274         421         420                 40.0093
       0.275         422         421                 40.0091
       0.276         423         422                  40.009
       0.277         424         423                 40.0084
       0.279         425         424                 40.0079
       0.280         426         425                 40.0075
       0.281         427         426                  40.007
       0.281         428         427                 40.0068
       0.283         429         428                 40.0064
       0.284         430         429                 40.0062
       0.285         431         430                 40.0059
       0.286         432         431                 40.0054
       0.287         433         432                 40.0052
       0.288         434         433                 40.0047
       0.290         435         434                 40.0046
       0.291         436         435                 40.0045
       0.292         437         436                 40.0042
       0.293         438         437                  40.004
       0.295         439         438                 40.0036
       0.296         440         439                 40.0035
       0.297         441         440                 40.0033
       0.299         442         441                 40.0031
       0.300         443         442                 40.0029
       0.301         444         443                 40.0028
       0.302         445         444                 40.0027
       0.304         446         445                 40.0026
       0.305         447         446                 40.0026
       0.306         448         447                 40.0025
       0.308         449         448                 40.0025
       0.309         450         449                 40.0022
       0.311         451         450                 40.0021
       0.312         452         451                 40.0019
       0.313         453         452                 40.0019
       0.315         454         453                 40.0014
       0.316         455         454                  40.001
       0.318         456         455                 40.0009
       0.319         457         456                 40.0008
       0.320         458         457                 40.0005
       0.322         459         458                 40.0004
       0.323         460         459                 40.0003
       0.325         461         460                 40.0002
       0.326         462         461                 40.0001
       0.327         463         462                 40.0001
       0.329         464         463                      40
       0.330         465         464                      40

Final statistics
----------------
Value:                        inf
Bound:                        40
Absolute optimality gap:      inf
Relative optimality gap (%):  inf
Time:                         0.331269

Solution
--------
Feasible:           0
Value:              0
Number of columns:  0
```

```shell
./bazel-bin/examples/multipleknapsack_main -v 1 -a "greedy" -i "../ordata/multipleknapsack/fukunaga2011/FK_1/random10_100_4_1000_1_1.txt"
```
```
==========================================
          ColumnGenerationSolver          
==========================================

Model
-----
Number of constraints:               110
Column lower bound:                  0
Column upper bound:                  1
Dummy column objective coefficient:  1

Algorithm
---------
Greedy

Parameters
----------
Time limit:            inf
Messages
    Verbosity level:   1
    Standard output:   1
    File path:         
    # streams:         0
Logger
    Has logger:        0
    Standard error:    0
    File path:         

Column generation
-----------------

        Time   Iteration   # columns                   Value
        ----   ---------   ---------                   -----
       0.001           1           0                       0
       0.002           2          10                    4739
       0.003           3          20                 8018.86
       0.004           4          30                   10313
       0.006           5          40                 13339.7
       0.007           6          50                 17030.4
       0.009           7          60                 18759.4
       0.011           8          70                 23570.8
       0.013           9          80                 26026.9
       0.014          10          90                   26797

Tree search
-----------

        Time       Value       Bound         Gap     Gap (%)                         Comment
        ----       -----       -----         ---     -------                         -------
       0.016        -inf       26797         inf         inf                          ndoe 0
       0.021        -inf       26797         inf         inf                          ndoe 1
       0.023        -inf       26797         inf         inf                          ndoe 2
       0.025        -inf       26797         inf         inf                          ndoe 3
       0.027        -inf       26797         inf         inf                          ndoe 4
       0.029        -inf       26797         inf         inf                          ndoe 5
       0.029        -inf       26797         inf         inf                          ndoe 6
       0.031        -inf       26797         inf         inf                          ndoe 7
       0.031        -inf       26797         inf         inf                          ndoe 8
       0.032       26797       26797 2.88856e-09        0.00                          ndoe 9

Final statistics
----------------
Value:                        26797
Bound:                        26797
Absolute optimality gap:      2.88856e-09
Relative optimality gap (%):  1.07794e-11
Time:                         0.0319889

Solution
--------
Feasible:           1
Value:              26797
Number of columns:  10
```

```shell
./bazel-bin/examples/vehicleroutingwithtimewindows_main -v 1 --input "../ordata/vehicleroutingwithtimewindows/solomon1987/RC101.txt" -c sol.txt -a limited-discrepancy-search -t 5
```
```
==========================================
          ColumnGenerationSolver          
==========================================

Model
-----
Number of constraints:               101
Column lower bound:                  0
Column upper bound:                  1
Dummy column objective coefficient:  1

Algorithm
---------
Limited discrepancy search

Parameters
----------
Time limit:            5
Messages
    Verbosity level:   1
    Standard output:   1
    File path:         
    # streams:         0
Logger
    Has logger:        0
    Standard error:    0
    File path:         

Column generation
-----------------

        Time   Iteration   # columns                   Value
        ----   ---------   ---------                   -----
       0.001           1           0                       0
       0.011           2          21                  2680.5
       0.021           3          47                    3063
       0.031           4          73                    3063
       0.040           5          99                    4712
       0.049           6         123                    5797
       0.057           7         145                 6834.75
       0.066           8         170                 6834.75
       0.074           9         195                    7083
       0.081          10         223                 8238.75
       0.088          11         244                 8510.25
       0.094          12         273                 10859.9
       0.102          13         302                   12247
       0.109          14         332                 13054.8
       0.117          15         364                 14306.7
       0.125          16         395                 15945.6
       0.132          17         443                 16305.2
       0.138          18         504                 17919.9
       0.143          19         546                 18341.3
       0.147          20         585                 18107.4
       0.153          21         623                 18587.4
       0.156          22         661                 18569.7
       0.162          23         702                 18781.9
       0.168          24         742                 19594.9
       0.176          25         781                 19203.7
       0.182          26         820                 19051.6
       0.190          27         857                   18766
       0.199          28         894                 18385.6
       0.208          29         927                 18175.8
       0.215          30         956                   18070
       0.224          31         989                 17843.7
       0.233          32        1016                 17619.3
       0.242          33        1050                 17524.5
       0.251          34        1084                 17126.2
       0.261          35        1117                   16943
       0.271          36        1145                 16737.6
       0.280          37        1184                 16642.5
       0.290          38        1216                   16585
       0.299          39        1245                 16559.9
       0.310          40        1277                   16355
       0.317          41        1306                   16353
       0.325          42        1336                 16316.8
       0.334          43        1369                 16269.5
       0.342          44        1404                 16252.6
       0.352          45        1443                 16183.8
       0.360          46        1473                 16168.9
       0.368          47        1507                   16159
       0.376          48        1538                 16157.2
       0.384          49        1566                   16134
       0.392          50        1598                 16124.7
       0.402          51        1628                 16082.5
       0.410          52        1658                   16076
       0.418          53        1694                 16064.8
       0.426          54        1731                 16064.4
       0.433          55        1768                 16064.4
       0.455          56        1831                 15991.8
       0.470          57        1859                 15991.8
       0.486          58        1887                 15955.7
       0.502          59        1916                 15947.1
       0.516          60        1947                 15939.7
       0.530          61        1978                 15938.6
       0.545          62        2010                 15933.9
       0.558          63        2043                 15924.2
       0.598          64        2102                 15884.2
       0.624          65        2126                 15858.5
       0.649          66        2153                 15854.6
       0.674          67        2182                 15852.5
       0.699          68        2213                 15852.2
       0.724          69        2243                 15852.1
       0.782          70        2299                 15840.9
       0.815          71        2329                 15840.9
       0.848          72        2359                 15840.9
       0.881          73        2391                 15840.9
       0.914          74        2418                 15840.9

Tree search
-----------

        Time       Value       Bound         Gap     Gap (%)                         Comment
        ----       -----       -----         ---     -------                         -------
       0.946         inf     15840.9         inf         inf           node 1 depth 0 disc 0
       1.075         inf     15840.9         inf         inf           node 2 depth 1 disc 0
       1.182         inf     15840.9         inf         inf           node 3 depth 2 disc 0
       1.247         inf     15840.9         inf         inf           node 4 depth 3 disc 0
       1.321         inf     15840.9         inf         inf           node 5 depth 4 disc 0
       1.374         inf     15840.9         inf         inf           node 6 depth 5 disc 0
       1.399         inf     15840.9         inf         inf           node 7 depth 6 disc 0
       1.439         inf     15840.9         inf         inf           node 8 depth 7 disc 0
       1.464         inf     15840.9         inf         inf           node 9 depth 8 disc 0
       1.481         inf     15840.9         inf         inf          node 10 depth 9 disc 0
       1.500         inf     15840.9         inf         inf         node 11 depth 10 disc 0
       1.513         inf     15840.9         inf         inf         node 12 depth 11 disc 0
       1.522         inf     15840.9         inf         inf         node 13 depth 12 disc 0
       1.526         inf     15840.9         inf         inf         node 14 depth 13 disc 0
       1.529         inf     15840.9         inf         inf         node 15 depth 14 disc 0
       1.531         inf     15840.9         inf         inf         node 16 depth 15 disc 0
       1.532       17547     15840.9     1706.06       10.77         node 17 depth 16 disc 0
       1.533       17547     15840.9     1706.06       10.77         node 19 depth 17 disc 1
       1.535       17547     15840.9     1706.06       10.77         node 21 depth 16 disc 1
       1.539       17525     15840.9     1684.06       10.63         node 22 depth 17 disc 1
       1.541       17525     15840.9     1684.06       10.63         node 24 depth 15 disc 1
       1.542       17525     15840.9     1684.06       10.63         node 25 depth 16 disc 1
       1.543       17525     15840.9     1684.06       10.63         node 26 depth 17 disc 1
       1.547       17525     15840.9     1684.06       10.63         node 28 depth 14 disc 1
       1.552       17525     15840.9     1684.06       10.63         node 29 depth 15 disc 1
       1.554       17525     15840.9     1684.06       10.63         node 30 depth 16 disc 1
       1.554       17525     15840.9     1684.06       10.63         node 31 depth 17 disc 1
       1.560       17525     15840.9     1684.06       10.63         node 33 depth 13 disc 1
       1.564       17525     15840.9     1684.06       10.63         node 34 depth 14 disc 1
       1.567       17525     15840.9     1684.06       10.63         node 35 depth 15 disc 1
       1.569       17525     15840.9     1684.06       10.63         node 36 depth 16 disc 1
       1.571       17525     15840.9     1684.06       10.63         node 37 depth 17 disc 1
       1.572       17525     15840.9     1684.06       10.63         node 38 depth 18 disc 1
       1.583       17525     15840.9     1684.06       10.63         node 40 depth 12 disc 1
       1.591       17525     15840.9     1684.06       10.63         node 41 depth 13 disc 1
       1.596       17525     15840.9     1684.06       10.63         node 42 depth 14 disc 1
       1.599       17525     15840.9     1684.06       10.63         node 43 depth 15 disc 1
       1.600       17525     15840.9     1684.06       10.63         node 44 depth 16 disc 1
       1.601       17525     15840.9     1684.06       10.63         node 45 depth 17 disc 1
       1.614       17525     15840.9     1684.06       10.63         node 47 depth 11 disc 1
       1.623       17525     15840.9     1684.06       10.63         node 48 depth 12 disc 1
       1.630       17525     15840.9     1684.06       10.63         node 49 depth 13 disc 1
       1.643       17525     15840.9     1684.06       10.63         node 50 depth 14 disc 1
       1.646       17525     15840.9     1684.06       10.63         node 51 depth 15 disc 1
       1.649       17525     15840.9     1684.06       10.63         node 52 depth 16 disc 1
       1.649       17444     15840.9     1603.06       10.12         node 53 depth 17 disc 1
       1.670       17444     15840.9     1603.06       10.12         node 55 depth 10 disc 1
       1.690       17444     15840.9     1603.06       10.12         node 56 depth 11 disc 1
       1.699       17444     15840.9     1603.06       10.12         node 57 depth 12 disc 1
       1.709       17444     15840.9     1603.06       10.12         node 58 depth 13 disc 1
       1.714       17444     15840.9     1603.06       10.12         node 59 depth 14 disc 1
       1.719       17444     15840.9     1603.06       10.12         node 60 depth 15 disc 1
       1.722       17444     15840.9     1603.06       10.12         node 61 depth 16 disc 1
       1.723       17228     15840.9     1387.06        8.76         node 62 depth 17 disc 1
       1.754       17228     15840.9     1387.06        8.76          node 64 depth 9 disc 1
       1.781       17228     15840.9     1387.06        8.76         node 65 depth 10 disc 1
       1.800       17228     15840.9     1387.06        8.76         node 66 depth 11 disc 1
       1.811       17228     15840.9     1387.06        8.76         node 67 depth 12 disc 1
       1.817       17228     15840.9     1387.06        8.76         node 68 depth 13 disc 1
       1.822       17228     15840.9     1387.06        8.76         node 69 depth 14 disc 1
       1.825       17228     15840.9     1387.06        8.76         node 70 depth 15 disc 1
       1.828       17228     15840.9     1387.06        8.76         node 71 depth 16 disc 1
       1.829       17228     15840.9     1387.06        8.76         node 72 depth 17 disc 1
       1.871       17228     15840.9     1387.06        8.76          node 74 depth 8 disc 1
       1.903       17228     15840.9     1387.06        8.76          node 75 depth 9 disc 1
       1.924       17228     15840.9     1387.06        8.76         node 76 depth 10 disc 1
       1.939       17228     15840.9     1387.06        8.76         node 77 depth 11 disc 1
       1.949       17228     15840.9     1387.06        8.76         node 78 depth 12 disc 1
       1.955       17228     15840.9     1387.06        8.76         node 79 depth 13 disc 1
       1.961       17228     15840.9     1387.06        8.76         node 80 depth 14 disc 1
       1.964       17228     15840.9     1387.06        8.76         node 81 depth 15 disc 1
       1.967       17228     15840.9     1387.06        8.76         node 82 depth 16 disc 1
       1.968       17228     15840.9     1387.06        8.76         node 83 depth 17 disc 1
       2.022       17228     15840.9     1387.06        8.76          node 85 depth 7 disc 1
       2.075       17228     15840.9     1387.06        8.76          node 86 depth 8 disc 1
       2.117       17228     15840.9     1387.06        8.76          node 87 depth 9 disc 1
       2.143       17228     15840.9     1387.06        8.76         node 88 depth 10 disc 1
       2.174       17228     15840.9     1387.06        8.76         node 89 depth 11 disc 1
       2.190       17228     15840.9     1387.06        8.76         node 90 depth 12 disc 1
       2.198       17228     15840.9     1387.06        8.76         node 91 depth 13 disc 1
       2.204       17228     15840.9     1387.06        8.76         node 92 depth 14 disc 1
       2.208       17228     15840.9     1387.06        8.76         node 93 depth 15 disc 1
       2.211       17228     15840.9     1387.06        8.76         node 94 depth 16 disc 1
       2.212       17228     15840.9     1387.06        8.76         node 95 depth 17 disc 1
       2.292       17228     15840.9     1387.06        8.76          node 97 depth 6 disc 1
       2.353       17228     15840.9     1387.06        8.76          node 98 depth 7 disc 1
       2.413       17228     15840.9     1387.06        8.76          node 99 depth 8 disc 1
       2.452       17228     15840.9     1387.06        8.76         node 100 depth 9 disc 1
       2.478       17228     15840.9     1387.06        8.76        node 101 depth 10 disc 1
       2.500       17228     15840.9     1387.06        8.76        node 102 depth 11 disc 1
       2.517       17228     15840.9     1387.06        8.76        node 103 depth 12 disc 1
       2.525       17228     15840.9     1387.06        8.76        node 104 depth 13 disc 1
       2.533       17228     15840.9     1387.06        8.76        node 105 depth 14 disc 1
       2.540       17228     15840.9     1387.06        8.76        node 106 depth 15 disc 1
       2.544       17228     15840.9     1387.06        8.76        node 107 depth 16 disc 1
       2.545       17228     15840.9     1387.06        8.76        node 108 depth 17 disc 1
       2.631       17228     15840.9     1387.06        8.76         node 110 depth 5 disc 1
       2.739       17228     15840.9     1387.06        8.76         node 111 depth 6 disc 1
       2.828       17228     15840.9     1387.06        8.76         node 112 depth 7 disc 1
       2.881       17228     15840.9     1387.06        8.76         node 113 depth 8 disc 1
       2.928       17228     15840.9     1387.06        8.76         node 114 depth 9 disc 1
       2.972       17228     15840.9     1387.06        8.76        node 115 depth 10 disc 1
       2.996       17228     15840.9     1387.06        8.76        node 116 depth 11 disc 1
       3.031       17228     15840.9     1387.06        8.76        node 117 depth 12 disc 1
       3.052       17228     15840.9     1387.06        8.76        node 118 depth 13 disc 1
       3.060       17228     15840.9     1387.06        8.76        node 119 depth 14 disc 1
       3.065       17228     15840.9     1387.06        8.76        node 120 depth 15 disc 1
       3.066       17108     15840.9     1267.06        8.00        node 121 depth 16 disc 1
       3.187       17108     15840.9     1267.06        8.00         node 123 depth 4 disc 1
       3.302       17108     15840.9     1267.06        8.00         node 124 depth 5 disc 1
       3.392       17108     15840.9     1267.06        8.00         node 125 depth 6 disc 1
       3.469       17108     15840.9     1267.06        8.00         node 126 depth 7 disc 1
       3.534       17108     15840.9     1267.06        8.00         node 127 depth 8 disc 1
       3.588       17108     15840.9     1267.06        8.00         node 128 depth 9 disc 1
       3.621       17108     15840.9     1267.06        8.00        node 129 depth 10 disc 1
       3.648       17108     15840.9     1267.06        8.00        node 130 depth 11 disc 1
       3.671       17108     15840.9     1267.06        8.00        node 131 depth 12 disc 1
       3.685       17108     15840.9     1267.06        8.00        node 132 depth 13 disc 1
       3.696       17108     15840.9     1267.06        8.00        node 133 depth 14 disc 1
       3.704       17108     15840.9     1267.06        8.00        node 134 depth 15 disc 1
       3.710       17108     15840.9     1267.06        8.00        node 135 depth 16 disc 1
       3.711       17108     15840.9     1267.06        8.00        node 136 depth 17 disc 1
       3.859       17108     15840.9     1267.06        8.00         node 138 depth 3 disc 1
       4.070       17108     15840.9     1267.06        8.00         node 139 depth 4 disc 1
       4.202       17108     15840.9     1267.06        8.00         node 140 depth 5 disc 1
       4.295       17108     15840.9     1267.06        8.00         node 141 depth 6 disc 1
       4.405       17108     15840.9     1267.06        8.00         node 142 depth 7 disc 1
       4.477       17108     15840.9     1267.06        8.00         node 143 depth 8 disc 1
       4.523       17108     15840.9     1267.06        8.00         node 144 depth 9 disc 1
       4.563       17108     15840.9     1267.06        8.00        node 145 depth 10 disc 1
       4.585       17108     15840.9     1267.06        8.00        node 146 depth 11 disc 1
       4.602       17108     15840.9     1267.06        8.00        node 147 depth 12 disc 1
       4.621       17108     15840.9     1267.06        8.00        node 148 depth 13 disc 1
       4.629       17108     15840.9     1267.06        8.00        node 149 depth 14 disc 1
       4.633       17108     15840.9     1267.06        8.00        node 150 depth 15 disc 1
       4.635       17108     15840.9     1267.06        8.00        node 151 depth 16 disc 1
       4.636       17108     15840.9     1267.06        8.00        node 152 depth 17 disc 1
       4.825       17108     15840.9     1267.06        8.00         node 154 depth 2 disc 1

Final statistics
----------------
Value:                        17108
Bound:                        15840.9
Absolute optimality gap:      1267.06
Relative optimality gap (%):  7.99861
Time:                         5.00937

Solution
--------
Feasible:           1
Value:              17108
Number of columns:  16

Checker
-------
Number of visited locations:    100 / 100
Number of duplicates:           0
Number of routes:               16 / 25
Number of overloaded vehicles:  0
Number of late visits:          0
Feasible:                       1
Total travel time:              17108
```

## Usage, C++ library

See examples.
