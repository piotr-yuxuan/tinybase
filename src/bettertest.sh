g++  -m32 -g -O1 -Wall -I./src ix_testkpg_2.cc -o ix_test2 -L../lib -lix -lrm -lpf
g++  -m32 -g -O1 -Wall -I./src ix_testTA.cc -o ix_test1 -L../lib -lix -lrm -lpf
rm test.out


echo "                                                  " >> test.out 2>&1
echo "T1++++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test1
./ix_test1 1 > test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T2+++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test2
./ix_test1 2 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T3++++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test3
./ix_test1 3 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T4++++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test4
./ix_test1 4 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T5++++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test5
./ix_test1 5 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T6++++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test6
./ix_test1 6 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T7++++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test7
./ix_test1 7 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T8++++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test8
./ix_test1 8 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T9++++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test9
./ix_test1 9 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T10+++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test10
./ix_test1 10 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T11+++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test11
./ix_test2 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "T12+++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo Test12
./ix_test1 11 >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
echo "EOF+++++++++++++++++++++++++++++++++++++++++++++++" >> test.out 2>&1
echo "                                                  " >> test.out 2>&1
