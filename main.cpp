#include "zipper/zipper.h"
#include <iostream>
#include <vector>

int main() {

    zipper::Zipper zip;

    

    zip.add("hello.txt", "Hello, world!");

    zip.save("out_test.zip");
}