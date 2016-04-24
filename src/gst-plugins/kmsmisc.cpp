#include "kmsmisc.h"
#include <libsoup/soup.h>
#include <fstream>


static bool load_from_url(const char *file_name, const char *url) {
   SoupSession *session;
   SoupMessage *msg;
   FILE *dst;
   std::cout << "soup A" << std::endl;
   session = soup_session_sync_new ();
   std::cout << "soup B" << std::endl;
   msg = soup_message_new ("GET", url);
   std::cout << "soup C" << std::endl;
   if(msg == NULL){
     g_object_unref (session);
     std::cout << "soup D" << std::endl;
     return false;
   }
   std::cout << "soup E" << std::endl;
   soup_session_send_message (session, msg);
   std::cout << "soup F" << std::endl;
   dst = fopen (file_name, "w+");
   std::cout << "soup G" << std::endl;
   if (dst == NULL) {
     g_object_unref (msg);
     g_object_unref (session);
     std::cout << "soup H" << std::endl;
     return false;
   }
   std::cout << "soup I" << std::endl;
   fwrite (msg->response_body->data, 1, msg->response_body->length, dst);
   fclose (dst);
   g_object_unref (msg);
   g_object_unref (session);
   std::cout << "soup J" << std::endl;
   return true;
}

static bool fetchModel(std::string modelUrl, std::string &modelName, std::string tmpdir){
  SoupURI *tst = soup_uri_new(modelUrl.c_str());
  if(SOUP_URI_IS_VALID(tst)) {
    std::cout << "valid uri yes" << std::endl;
    modelName = tmpdir + "/" + modelName; 
    std::cout << "fetch model: " << modelUrl << " to:" << modelName << std::endl;
    if(load_from_url(modelName.c_str(), modelUrl.c_str()) == false){
      std::cout<<"Bizarre loading of 3DModel" << std::endl;
      return false;
    }
  } 
  else {
    std::cout << "valid uri no" << std::endl;
    return false;
  }
  return true;
}

static bool splitModel(std::string modelUrl, std::string &modelName, std::string &modelSuffix){
  unsigned found = modelUrl.find_last_of("/\\");
  if(found == std::string::npos){
    std::cout << "bizarre model uri " << std::endl;
    return false;
  }
  //modelName = "/tmp/" + modelUrl.substr(found+1);
  modelName = modelUrl.substr(found+1);
  
#if 1
  found = modelName.find_last_of(".");
  if(found == std::string::npos){
    std::cout << "bizarre model suffix " << std::endl;
    return false;
  }
  modelSuffix = modelName.substr(found+1);
#endif
return true;
  //  std::cout << "Try to load model: " << modelUrl << " from cache:" << modelLocal << " suffix:" << modelSuffix << std::endl;
}

std::string loadPlanar(std::string modelUrl){
  
  std::cout << "LoadPlanar beginBlue: " << modelUrl << std::endl;

  std::string modelName = "";
  std::string modelSuffix = "";

  if(splitModel(modelUrl, modelName, modelSuffix) == false){
    std::cout << "Bizarre local model skippedA:" << modelUrl << std::endl;
    return NULL;
  }
  
  std::ifstream infile(modelUrl.c_str());
  if(infile.good()){
    std::cout << "LoadPlanar A: " << modelUrl << std::endl;
    return modelUrl;
  }
  
  char tmpbase[] = "/tmp/ar_XXXXXX";
  std::string tmpdir = mkdtemp(tmpbase);
  if(fetchModel(modelUrl, modelName, tmpdir) == false){
    std::cout << "Bizarre texture skippedBtxt:" << modelUrl << std::endl;
    return NULL;
  }

  std::ifstream infile2(modelName.c_str());
  if(infile2.good()){
    std::cout << "LoadPlanar end: " << modelName << std::endl;
    return modelName;
  }

  std::cout << "LoadPlanar failed: " << modelUrl << " " << modelName << std::endl;  

  return NULL;
}
