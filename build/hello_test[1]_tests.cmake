add_test( HelloTest.BasicAssertions /u/wzhang/cs429/prog3/build/hello_test [==[--gtest_filter=HelloTest.BasicAssertions]==] --gtest_also_run_disabled_tests)
set_tests_properties( HelloTest.BasicAssertions PROPERTIES WORKING_DIRECTORY /u/wzhang/cs429/prog3/build)
set( hello_test_TESTS HelloTest.BasicAssertions)
