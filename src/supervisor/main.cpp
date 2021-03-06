#include "supervisor.h"

int main(int argc, char** argv) {

    if(argc < 2) {
        cout<< "usage: test_supervisor keyword" << endl;
        return 0;
    }

    Supervisor supervisor = Supervisor("../supervisor_storage/agent_info");
    supervisor.SearchKeyword(argv[1]);
    return 0;
}
