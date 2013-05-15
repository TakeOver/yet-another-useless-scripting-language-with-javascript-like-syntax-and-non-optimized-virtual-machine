#include "../nls.hpp"
int main(int argc, char const *argv[])
{
        nls::NlsApi* api = new nls::NlsApi();
        api->Require("lfm.nls");
        api->setToUD("Embed",nls::Value(true));
        api->Execute();
        api->call("game","Play");
        api->call("game","Show");
        delete api;
        return 0;
}