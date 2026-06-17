#include <iostream>
#include "test_protobuf.pb.h"

int main() {
    Test test;
    //  test.set_file_name(__FILE__);
    //  test.set_line(__LINE__);
     test.set_test_id(111);
     test.set_msg("A");

     std::string data;
     test.SerializeToString(&data);
     for (char c : data) {
        printf("%02x", c);
     }
     std::cout << std::endl;

     Test output;
     output.ParseFromString(data);

     std::cout << "FILE_NAME: " << output.file_name() << std::endl;
     std::cout << "TEST_ID: " << output.test_id() << std::endl;
     std::cout << "MSG: " << output.msg() << std::endl;


}
