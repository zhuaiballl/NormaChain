#include "agent.h"
using namespace std;

Agent::Agent() {
    mNumContract = 0;
}

Agent::Agent(string agent_info_path, string contract_root_dir) {
    //check if keys already exist
    init_pbc_param_pairing(mParam, mPairing);
    double P = mpz_get_d(mPairing->r);
    KeyGen(&mKey, mParam, mPairing);
    mNumContract = 0;
    this->Load_Agent_Info(agent_info_path);
    Set_Contract_Root(contract_root_dir);
    __load_contract();
}

void Agent::__save_key(string key_file_path) {

    KeyGen(&mKey, mParam, mPairing);

    vector<string> key_str;
    //save private key
    int priv_len = element_length_in_bytes(mKey.priv);
    unsigned char private_c[priv_len];
    element_to_bytes(private_c, mKey.priv);
    string priv_str((char*)private_c);
    key_str.push_back(priv_str);

    //save g
    int g_len = element_length_in_bytes(mKey.pub.g);
    unsigned char g_c[g_len];
    element_to_bytes(g_c, mKey.pub.g);
    string g_str((char*)g_c);
    key_str.push_back(g_str);

    //save h
    int h_len = element_length_in_bytes(mKey.pub.h);
    unsigned char h_c[h_len];
    element_to_bytes(h_c, mKey.pub.h);
    string h_str((char*)h_c);
    key_str.push_back(h_str);

    //save key to key file
    stringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << key_str;
    ofstream key_out_file(key_file_path);
    key_out_file << archive_stream.str();
    key_out_file.close();
}

void Agent::__load_key(string key_file_path) {
    vector<string> key_str;

    std::ifstream ifs(key_file_path);
    std::string key_content( (std::istreambuf_iterator<char>(ifs) ),
                           (std::istreambuf_iterator<char>()) );
    ifs.close();

    stringstream iarchive_stream;
    iarchive_stream << key_content;
    boost::archive::text_iarchive iarchive(iarchive_stream);
    iarchive >> key_str;

    element_t key_priv;
    element_init_G1(key_priv, mPairing);
    element_from_bytes(key_priv, (unsigned char*) key_str[0].c_str());

    element_t g;
    element_init_G1(g, mPairing);
    element_from_bytes(g, (unsigned char*) key_str[1].c_str());

    element_t h;
    element_init_G1(h, mPairing);
    element_from_bytes(h, (unsigned char*) key_str[2].c_str());

    element_init_same_as(mKey.priv, key_priv);
    element_init_same_as(mKey.pub.g, g);
    element_init_same_as(mKey.pub.h, h);
}

void Agent::Load_Agent_Info(string path) {
    ConfigParser agent_parser= ConfigParser();
    agent_parser.OpenFile(path);
    map<string, string> agent_map = agent_parser.Parse();

    if (agent_map.size() > 0) {
        mAddr = agent_map["ADDR"];
        mIPAddr = agent_map["IP_ADDR"];
        mOpenPort = agent_map["OPENPORT"];
    }
    else {
        cout << "Parsing Approver Info fails!" << endl;
    }
}

void Agent::__encrypt_contract(Contract contract) {
    contract.setTransactionID((uint64_t)mContractList.size());
    uint64_t Transaction_ID = contract.getTransactionID();
    string buyer_addr = contract.getBuyerAddr();
    string seller_addr = contract.getSellerAddr();
    string product = contract.getProductInfo();
    double price = contract.getPrice();
    string description = contract.getDescription();
    vector<string> description_words;

    vector<element_s> trapdoor_list;
    description_words.clear();
    vector<vector<unsigned char>> contract_trapdoor_list;
    contract_trapdoor_list.clear();

    // parse desription based on
    std::stringstream description_ss(description);
    string one_word_description;
    while (description_ss.good())
    {
        getline(description_ss, one_word_description, ' ' );
        description_words.push_back(one_word_description);
    }

    // encrypt transaction id
    {
        char *hashedW = (char*) malloc(sizeof(char)*SHA512_DIGEST_LENGTH*2+1);
        element_t Tw;    // trapdoor word
        element_t H1_W1;
        string Transaction_ID_str = std::to_string(Transaction_ID);
        char* Transaction_ID_c = (char*) Transaction_ID_str.c_str();
        sha512(Transaction_ID_c, (int)strlen(Transaction_ID_c), hashedW);
        element_init_G1(H1_W1, mPairing);
        element_from_hash(H1_W1, hashedW, (int)strlen(hashedW));
        Trapdoor(Tw, mPairing, mKey.priv, H1_W1);
        free(hashedW);
        hashedW = NULL;
        element_s tmp = {Tw->field, Tw->data};
        trapdoor_list.push_back(tmp);

        int len = element_length_in_bytes(Tw);
        unsigned char data_tmp[len];
        element_to_bytes(data_tmp, Tw);
        vector<unsigned char> data_vec_tmp(data_tmp, data_tmp + len);
        contract_trapdoor_list.push_back(data_vec_tmp);
    }


    // encrypt buyer addr
    {
        char *hashedW = (char*) malloc(sizeof(char)*SHA512_DIGEST_LENGTH*2+1);
        element_t Tw;    // trapdoor word
        element_t H1_W1;
        char* buyer_addr_c = (char*) buyer_addr.c_str();
        sha512(buyer_addr_c, (int)strlen(buyer_addr_c), hashedW);
        element_init_G1(H1_W1, mPairing);
        element_from_hash(H1_W1, hashedW, (int)strlen(hashedW));
        Trapdoor(Tw, mPairing, mKey.priv, H1_W1);
        free(hashedW);
        hashedW = NULL;
        element_s tmp = {Tw->field, Tw->data};
        trapdoor_list.push_back(tmp);

        int len = element_length_in_bytes(Tw);
        unsigned char data_tmp[len];
        element_to_bytes(data_tmp, Tw);
        vector<unsigned char> data_vec_tmp(data_tmp, data_tmp + len);
        contract_trapdoor_list.push_back(data_vec_tmp);
    }

    // encrypt seller addr
    {
        char *hashedW = (char*) malloc(sizeof(char)*SHA512_DIGEST_LENGTH*2+1);
        element_t Tw;    // trapdoor word
        element_t H1_W1;
        char* seller_addr_c = (char*) seller_addr.c_str();
        sha512(seller_addr_c, (int)strlen(seller_addr_c), hashedW);
        element_init_G1(H1_W1, mPairing);
        element_from_hash(H1_W1, hashedW, (int)strlen(hashedW));
        Trapdoor(Tw, mPairing, mKey.priv, H1_W1);
        free(hashedW);
        hashedW = NULL;
        element_s tmp = {Tw->field, Tw->data};
        trapdoor_list.push_back(tmp);

        int len = element_length_in_bytes(Tw);
        unsigned char data_tmp[len];
        element_to_bytes(data_tmp, Tw);
        vector<unsigned char> data_vec_tmp(data_tmp, data_tmp + len);
        contract_trapdoor_list.push_back(data_vec_tmp);
    }


    // encrypt product
    {
        char *hashedW = (char*) malloc(sizeof(char)*SHA512_DIGEST_LENGTH*2+1);
        element_t Tw;    // trapdoor word
        element_t H1_W1;
        string price_str = std::to_string(price);
        char* price_c = (char*) price_str.c_str();
        sha512(price_c, (int)strlen(price_c), hashedW);
        element_init_G1(H1_W1, mPairing);
        element_from_hash(H1_W1, hashedW, (int)strlen(hashedW));
        Trapdoor(Tw, mPairing, mKey.priv, H1_W1);
        free(hashedW);
        hashedW = NULL;
        element_s tmp = {Tw->field, Tw->data};
        trapdoor_list.push_back(tmp);

        int len = element_length_in_bytes(Tw);
        unsigned char data_tmp[len];
        element_to_bytes(data_tmp, Tw);
        vector<unsigned char> data_vec_tmp(data_tmp, data_tmp + len);
        contract_trapdoor_list.push_back(data_vec_tmp);
    }

    // encrypt price
    {
        char *hashedW = (char*) malloc(sizeof(char)*SHA512_DIGEST_LENGTH*2+1);
        element_t Tw;    // trapdoor word
        element_t H1_W1;
        char* product_char = (char*) product.c_str();
        sha512(product_char, (int)strlen(product_char), hashedW);
        element_init_G1(H1_W1, mPairing);
        element_from_hash(H1_W1, hashedW, (int)strlen(hashedW));
        Trapdoor(Tw, mPairing, mKey.priv, H1_W1);
        free(hashedW);
        hashedW = NULL;
        element_s tmp = {Tw->field, Tw->data};
        trapdoor_list.push_back(tmp);

        int len = element_length_in_bytes(Tw);
        unsigned char data_tmp[len];
        element_to_bytes(data_tmp, Tw);
        vector<unsigned char> data_vec_tmp(data_tmp, data_tmp + len);
        contract_trapdoor_list.push_back(data_vec_tmp);
    }

    // encrypt description
    {
        for (int i = 0; i < description_words.size(); i++) {
            char *hashedW = (char*) malloc(sizeof(char)*SHA512_DIGEST_LENGTH*2+1);
            element_t Tw;    // trapdoor word
            element_t H1_W1;
            char* description_word = (char*) description_words[i].c_str();
            sha512(description_word, (int)strlen(description_word), hashedW);
            element_init_G1(H1_W1, mPairing);
            element_from_hash(H1_W1, hashedW, (int)strlen(hashedW));
            Trapdoor(Tw, mPairing, mKey.priv, H1_W1);
            free(hashedW);
            hashedW = NULL;
            element_s tmp = {Tw->field, Tw->data};
            trapdoor_list.push_back(tmp);

            int len = element_length_in_bytes(Tw);
            unsigned char data_tmp[len];
            element_to_bytes(data_tmp, Tw);
            vector<unsigned char> data_vec_tmp(data_tmp, data_tmp + len);
            contract_trapdoor_list.push_back(data_vec_tmp);
        }
    }

    this->mContract2TrapdoorMap.insert(pair<uint64_t, vector<element_s>>(contract.getTransactionID(), trapdoor_list));
//    __save_encryptedcontract(contract_trapdoor_list);
}

void Agent::__save_encryptedcontract(vector<vector<unsigned char>> trapdoor_list) {
	stringstream archive_stream;
	boost::archive::text_oarchive archive(archive_stream);
	archive << trapdoor_list;
    ofstream contract_out_file(mContractRootDir + "/contract" + to_string(mContractList.size()) + ".peksct");
	contract_out_file << archive_stream.str();
	contract_out_file.close();
}

void Agent::__load_encryptedcontract() {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (mContractRootDir.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            string contract_file_name(ent->d_name);
            if (contract_file_name == "." || contract_file_name == "..") {
                continue;
            }
            string full_path = mContractRootDir + "/" + contract_file_name;
            std::ifstream ifs(full_path);
            std::string contract_content( (std::istreambuf_iterator<char>(ifs) ),
                                   (std::istreambuf_iterator<char>()    ) );
            ifs.close();

            stringstream iarchive_stream;
            iarchive_stream << contract_content;
            boost::archive::text_iarchive iarchive(iarchive_stream);
            vector<vector<unsigned char>> encrypted_contract;
            iarchive >> encrypted_contract;

            vector<element_s> tmp_vec;
            for(int i=0; i<encrypted_contract.size(); i++) {
                element_t tmp_et;
                element_init_G1(tmp_et, mPairing);
                int len = element_length_in_bytes(tmp_et);
                element_from_bytes(tmp_et, encrypted_contract[i].data());
                element_s tmp_es = {tmp_et->field, tmp_et->data};
                tmp_vec.push_back(tmp_es);
            }

            this->mContract2TrapdoorMap.insert(pair<uint64_t, vector<element_s>>((uint64_t)mContractList.size(), tmp_vec));

        }
        closedir (dir);
    }
    else {
        perror ("Fail to open contract directory");
    }
}

void Agent::__save_contract(Contract contract) {
    string file_content = contract.genContractFileStr();
    std::ofstream out(mContractRootDir + "/contract" + to_string(mContractList.size()) + ".ct");
    out << file_content;
    out.close();
}

void Agent::__load_contract() {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (mContractRootDir.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            string contract_file_name(ent->d_name);
            if (contract_file_name == "." || contract_file_name == "..") {
                continue;
            }
            string full_path = mContractRootDir + "/" + contract_file_name;
            Contract contract = Contract(full_path);
            mContractList.push_back(contract);
            cout << contract.getDescription() << endl;
            __encrypt_contract(contract);
        }
        closedir (dir);
    }
    else {
        perror ("Fail to open contract directory");
    }
}


void Agent::__recv_contract(HttpServer &server) {
    server.resource["^/contract$"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        try {
            string recv_string = request->content.string();
            stringstream iarchive_stream;
            iarchive_stream << recv_string;
            boost::archive::text_iarchive iarchive(iarchive_stream);
            Contract recv_contract = Contract();
            iarchive >> recv_contract;

            string response_str = "Contract received!";
            cout << response_str << endl;
            *response << "HTTP/1.1 200 OK\r\n"
                      << "Content-Length: " << response_str.length() << "\r\n\r\n"
                      << response_str;

            uint64_t Transaction_ID = (uint64_t)mContractList.size();
            recv_contract.setTransactionID(Transaction_ID);
            __encrypt_contract(recv_contract);
            mContractList.push_back(recv_contract);
            __save_contract(recv_contract);
        }
        catch(const exception &e) {
          *response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n"
                    << e.what();
        }
    };
}

vector<uint64_t> Agent::__search_keyword(string keyword) {
    pair<uint64_t, vector<element_s>> it;
    vector<uint64_t> Transaction_IDs;
    BOOST_FOREACH(it, mContract2TrapdoorMap) {
        uint64_t Transaction_ID;
        Transaction_ID = it.first;
        vector<element_s> trapdoor_list = mContract2TrapdoorMap[Transaction_ID];
        for(int i=0; i<trapdoor_list.size(); i++) {
            element_t Tw = {trapdoor_list[i].field, trapdoor_list[i].data};
            char* keyword_c = (char*) keyword.c_str();
            int match = Test(keyword_c, (int)strlen(keyword_c), &mKey.pub, Tw, mPairing);
            if(match) {
                Transaction_IDs.push_back(Transaction_ID);
                break;
            }
        }

    }
    return Transaction_IDs;
}

void Agent::__recv_searchrequest(HttpServer &server) {
    thread search_request_thread([this, &server] {
    server.resource["^/searchrequest$"]["POST"] = [this](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        try {
            ptree pt;
            read_json(request->content, pt);
            string keyword;
            keyword = pt.get<string>("keyword");
            cout << "Recieve a search request with keyword " << keyword << endl;
            const clock_t begin_time = clock();
            //serialize the transaction id vector and send to supervisor
            vector<uint64_t> found_trans_id_list = __search_keyword(keyword);
            stringstream archive_stream;
            boost::archive::text_oarchive archive(archive_stream);
            archive << found_trans_id_list;
            cout << "Found " << found_trans_id_list.size() << " records for keyword " << keyword <<"." << endl;
            std::cout << "The search time is " << float( clock () - begin_time ) /  CLOCKS_PER_SEC << std::endl;

            *response << "HTTP/1.1 200 OK\r\n"
                      << "Content-Length: " << archive_stream.str().length() << "\r\n\r\n"
                      << archive_stream.str();
        }
        catch(const exception &e) {
          *response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n"
                    << e.what();
        }
    };
    });
    search_request_thread.detach();
}

void Agent::serve() {
    HttpServer server;
    server.config.port = stoi(mOpenPort);
    this->__recv_searchrequest(server);
    this->__recv_contract(server);
    thread server_thread([&server]() {
        // Start server
        server.start();
    });
    server_thread.join();
}

void Agent::test() {
    Contract contract1 = Contract(0, "0x123", "0x152", 100, std::time(nullptr), "Bought a bread", "bread");
    Contract contract2 = Contract(1, "0x111", "0x287", 100, std::time(nullptr), "Bought some drugs", "drug");
    Contract contract3 = Contract(2, "0x111", "0x287", 100, std::time(nullptr), "Bought a strawberry", "strawberry");
    __save_contract(contract1);
    __save_contract(contract2);
    __save_contract(contract3);
    __encrypt_contract(contract1);
    __encrypt_contract(contract2);
    __encrypt_contract(contract3);
    HttpServer server;
    server.config.port = stoi(mOpenPort);
    this->__recv_searchrequest(server);
    this->__recv_contract(server);
    thread server_thread([&server]() {
        // Start server
        server.start();
    });
    server_thread.join();
}

void Agent::setAddr(string Addr) {
    mAddr = Addr;
}

void Agent::setIPAddr(string IPAddr) {
    mIPAddr = IPAddr;
}

void Agent::setOpenPort(string OpenPort) {
    mOpenPort = OpenPort;
}

string Agent::getAddr() {
    return mAddr;
}

string Agent::getIPAddr() {
    return mIPAddr;
}

string Agent::getOpenPort() {
    return mOpenPort;
}

void Agent::Set_Contract_Root(string contract_root_dir) {
    mContractRootDir = contract_root_dir;
}

