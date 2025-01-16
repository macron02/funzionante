#include "soapDeviceBindingProxy.h"
#include "soapMediaBindingProxy.h"
#include "soapThermalBindingProxy.h"
#include "soapPTZBindingProxy.h"
#include "soapH.h"
#include "soapStub.h"
#include "soapPullPointSubscriptionBindingProxy.h"
#include "soapRemoteDiscoveryBindingProxy.h" 

#include "wsddapi.h"
#include "wsseapi.h"
#include "wsdd.nsmap"
#include "httpda.h"
   
#include <regex>
#include <iostream>
#include <string>
#include <thread>  // Necessario per std::this_thread::sleep_for
#include <chrono>  // Necessario per std::chrono::seconds
#include <vector>



#define USERNAME "admin"
#define PASSWORD "Password_01"
#define HOSTNAME "192.168.20.104"

std::vector<std::string> _pullpointEndpoints;

//int CRYPTO_thread_setup();
void CRYPTO_thread_cleanup();


void report_error (struct soap *soap, int lineno)
{
  std::cerr << "-----------------------------------------" << std::endl;
  std::cerr << "Oops, something went wrong: error: " << soap->error << " at line: " << lineno << std::endl;
  soap_stream_fault(soap, std::cerr);
  std::cerr << "-----------------------------------------" << std::endl;
  SOAP_OK;
}

static void set_device_endpoint(DeviceBindingProxy *dev)
{
	static char soap_endpoint[1024];
	sprintf(soap_endpoint, "http://%s/onvif/device_service", HOSTNAME);

	dev->soap_endpoint = soap_endpoint;
}

static void set_pull_endpoint(PullPointSubscriptionBindingProxy *dev)
{
  static char soap_endpoint[1024];
	sprintf(soap_endpoint, "http://%s/onvif/device_service", HOSTNAME);

	dev->soap_endpoint = soap_endpoint;

}

static void set_media_endpoint(MediaBindingProxy *dev)
{
	static char soap_endpoint[1024];
	sprintf(soap_endpoint, "http://%s/onvif/device_service", HOSTNAME);

	dev->soap_endpoint = soap_endpoint;
}

static void set_thermal_endpoint(ThermalBindingProxy *dev)
{
	static char soap_endpoint[1024];
	sprintf(soap_endpoint, "http://%s/onvif/device_service", HOSTNAME);

	dev->soap_endpoint = soap_endpoint;
}

// to set the timestamp and authentication credentials in a request message
void set_credentials(struct soap *soap)
{
  
  soap_wsse_add_Timestamp(soap, "Time", 10);
  if (soap_wsse_add_UsernameTokenDigest(soap, "auth", USERNAME, PASSWORD))
    report_error(soap, __LINE__);

}

// to check if an ONVIF service response was signed with WS-Security (when enabled)
void check_response(struct soap *soap)
{

}

// to download a snapshot and save it locally in the current dir as image-1.jpg, image-2.jpg, image-3.jpg ...
struct soap* authenticate(const char* endpoint) {
    // Creazione del contesto SOAP
    struct soap* soap = soap_new();
    if (!soap) {
        std::cerr << "Errore nella creazione del contesto SOAP" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Registrazione del plugin HTTP Digest Authentication
    soap_register_plugin(soap, http_da);

    // Configurazione dei timeout
    soap->connect_timeout = soap->recv_timeout = soap->send_timeout = 10; // 10 secondi

    // Configurazione SSL (se necessario)
    if (soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL, NULL, NULL)) {
        std::cerr << "Errore nella configurazione SSL" << std::endl;
        soap_destroy(soap);
        soap_end(soap);
        soap_free(soap);
        exit(EXIT_FAILURE);
    }

    // Primo tentativo senza autenticazione
    if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap)) {
        std::cout << "HTTP status: " << soap->status << " | Auth realm: "
                  << (soap->authrealm ? soap->authrealm : "None") << std::endl;

        if (soap->status == 401 && soap->authrealm) {
            // Tentativo con Digest Authentication
            struct http_da_info info;
            std::cout << "Tentativo con Digest Authentication..." << std::endl;
            soap_strdup(soap, soap->authrealm);
            http_da_save(soap, &info, soap->authrealm, USERNAME, PASSWORD);

            if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap)) {
                std::cerr << "Errore con Digest Authentication..." << std::endl;
                http_da_release(soap, &info);
                soap_destroy(soap);
                soap_end(soap);
                soap_free(soap);
                exit(EXIT_FAILURE);
            } else {
                std::cout << "Autenticazione con Digest Authentication riuscita!" << std::endl;
            }

            http_da_release(soap, &info);
        } else {
            // Tentativo con HTTP Basic Authentication
            std::cout << "Tentativo con HTTP Basic Authentication..." << std::endl;
            soap->userid = USERNAME;
            soap->passwd = PASSWORD;

            if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap)) {
                std::cerr << "HTTP Basic Authentication fallita." << std::endl;
                soap_destroy(soap);
                soap_end(soap);
                soap_free(soap);
                exit(EXIT_FAILURE);
            } else {
                std::cout << "Autenticazione con HTTP Basic Authentication riuscita!" << std::endl;
            }
        }
    } else {
        std::cout << "Autenticazione iniziale riuscita!" << std::endl;
    }

    // Termina la ricezione per completare il contesto SOAP
    soap_end_recv(soap);

    // Restituisce il contesto autenticato
    return soap;
}

void save_snapshot(int i, const char* endpoint) {
    char filename[32];
    snprintf(filename, sizeof(filename), "image-%d.jpg", i);

    FILE* fd = fopen(filename, "wb");
    if (!fd) {
        std::cerr << "Cannot open " << filename << " for writing" << std::endl;
        exit(EXIT_FAILURE);
    }

    struct soap* soap = authenticate(endpoint);

    std::cout << "Retrieving " << filename << " from " << endpoint << std::endl;

    size_t imagelen;
    char* image = soap_http_get_body(soap, &imagelen);
    if (!image) {
        std::cerr << "Errore nel recupero del corpo HTTP" << std::endl;
        report_error(soap, __LINE__);
    }

    fwrite(image, 1, imagelen, fd);
    fclose(fd);

    // Cleanup
    soap_destroy(soap);
    soap_end(soap);
    soap_free(soap);
}

// da commentare
void get_pullpoints(struct soap *soap, const char *event_service_endpoint)
{
  ///////////
  PullPointSubscriptionBindingProxy proxyEvent(soap);

    // Imposta l'endpoint del servizio eventi
    proxyEvent.soap_endpoint = event_service_endpoint;
    set_credentials(soap);
    set_pull_endpoint(&proxyEvent);
    ////////////


    // Preparazione della richiesta
    _tev__GetEventProperties getEventProperties;
    _tev__GetEventPropertiesResponse getEventPropertiesResponse;

    // Impostazione delle credenziali
    set_credentials(soap);

    // Chiamata al servizio GetEventProperties

    //if (proxyEvent.CreatePullPointSubscription(&createSubscription, createSubscriptionResponse)

    if (proxyEvent.GetEventProperties(&getEventProperties, getEventPropertiesResponse))
    {
        report_error(soap, __LINE__);
        return;
    }

    // Verifica della risposta
    check_response(soap);

    for (auto item : getEventPropertiesResponse.TopicNamespaceLocation) {
      std::cout << "Suported topic: " << item << std::endl;
    }

// funzioni che non mi servono al momento
    //for (auto item : getEventPropertiesResponse.FixedTopicSet) {
    //  std::cout << "Suported Fixedtopic: " << item << std::endl;
    //}

    //for (auto item : getEventPropertiesResponse.TopicExpressionDialect) {
    //  std::cout << "Suported ExpressiontopicDialect: " << item << std::endl;
    //}

    for (auto item : getEventPropertiesResponse.MessageContentFilterDialect) {
      std::cout << "Suported Filtertopic: " << item << std::endl;
    }

    for (auto item : getEventPropertiesResponse.ProducerPropertiesFilterDialect) {
      std::cout << "Suported produceFiltertopic: " << item << std::endl;
    }

    for (auto item : getEventPropertiesResponse.MessageContentSchemaLocation ) {
      std::cout << "Suported SchemaLocation: " << item << std::endl;
    }

    

    // Estrarre e stampare i PullPoint dal TopicSet da commentare dopo aver trovato il necessario
     if (getEventPropertiesResponse.wstop__TopicSet)
     {
         std::cout << "PullPoints disponibili:\n";
         for (const auto &topic : getEventPropertiesResponse.wstop__TopicSet->__any)

//////////////////

         {
             if (topic)
             {
                 std::cout << "  " << topic << std::endl;
             }
         }
     }
     else
     {
         std::cerr << "Errore: Nessun PullPoint trovato nel TopicSet." << std::endl;
     }
}
//thermal al momento da non usare
/*
void get_thermal_info(struct soap *soap, const std::string &profileToken) {
    // Inizializza il proxy per il servizio Media
    MediaBindingProxy proxyMedia(soap);
    set_media_endpoint(&proxyMedia);

    // Richiedi le informazioni sul VideoSource per il profilo dato
    _trt__GetVideoSources getVideoSources;
    _trt__GetVideoSourcesResponse getVideoSourcesResponse;
    set_credentials(soap);
    if (proxyMedia.GetVideoSources(&getVideoSources, getVideoSourcesResponse)) {
        report_error(soap, __LINE__);
        return;
    }
    check_response(soap);
    */

/*
    // Trova il VideoSourceToken corrispondente al profileToken
    std::string videoSourceToken;
    for (const auto &videoSource : getVideoSourcesResponse.VideoSources) {
        //if (videoSource->token == profileToken) {
          //  videoSourceToken = videoSource->token;
            std::cout<< "video source token: " << videoSourceToken << ", token: " << videoSource->token << std :: endl;
            //break;

             // Inizializza il proxy per il servizio termico
    ThermalBindingProxy proxyThermal(soap);
    set_thermal_endpoint(&proxyThermal);

            // Richiedi la configurazione di radiometria
    _tth__GetRadiometryConfiguration getRadiometryConfig;
    _tth__GetRadiometryConfigurationResponse getRadiometryConfigResponse;
   // getRadiometryConfigResponse.ConfigurationToken
    std::cout<< "secondo tentativo, video source token: " << videoSourceToken << ", token: " << videoSource->token << std :: endl;
    set_credentials(soap);
    std:: cout << "cosa fa questo???" << getRadiometryConfig.VideoSourceToken << std::endl;
    getRadiometryConfig.VideoSourceToken =  videoSource -> token; // profileToken;
    std::cout<< "terzo tentativo, video source token: " << videoSourceToken << ", token: " << videoSource->token << std :: endl;
    //set_credentials(soap);
    std::cout<< "quarto tentativo, video source token: " << videoSourceToken << ", token: " << videoSource->token << std :: endl;
   // if (proxyThermal.GetRadiometryConfiguration(&getRadiometryConfig, getRadiometryConfigResponse)) {
     //   report_error(soap, __LINE__);
       // return;
       /////////////////////
 // _tth__GetRadiometryConfiguration getRadiometryConfig;
 //   _tth__GetRadiometryConfigurationResponse getRadiometryConfigResponse;
     _trt__GetProfiles GetProfiles;
  _trt__GetProfilesResponse GetProfilesResponse;
  // getRadiometryConfig.VideoSourceToken = GetProfilesResponse.Profiles[i]->token;
   set_credentials(soap);
        
       // ThermalBindingProxy proxyThermal(soap);
   // set_thermal_endpoint(&proxyThermal);

   std::cout << "sto nel test " << proxyThermal.soap_endpoint << " - " <<std::endl;

   /*if (proxyThermal.getRadiometryConfig(&getRadiometryConfig, getRadiometryConfigResponse)) {
       report_error(soap, __LINE__);
   }
   */
   /*
   check_response(soap);

   if (getRadiometryConfigResponse.Configuration) {
       std::cout << "test 2: " << getRadiometryConfigResponse.Configuration << std::endl;
       
   } else {
       std::cerr << "Failed to test" << getRadiometryConfigResponse.Configuration << std::endl;
   }
/////////////////////////////////////
*/ 
/*

       if (getRadiometryConfigResponse.Configuration) {
        const auto &config = getRadiometryConfigResponse.Configuration;
        std::cout << "ReflectedAmbientTemperature: " << config->RadiometryGlobalParameters->ReflectedAmbientTemperature << std::endl;
        std::cout << "Emissivity: " << config->RadiometryGlobalParameters->Emissivity << std::endl;
        std::cout << "DistanceToObject: " << config->RadiometryGlobalParameters->DistanceToObject << std::endl;
        if (config->RadiometryGlobalParameters->RelativeHumidity)
            std::cout << "RelativeHumidity: " << *config->RadiometryGlobalParameters->RelativeHumidity << std::endl;
        if (config->RadiometryGlobalParameters->AtmosphericTemperature)
            std::cout << "AtmosphericTemperature: " << *config->RadiometryGlobalParameters->AtmosphericTemperature << std::endl;
    } else {
        std::cerr << "Configurazione di radiometria non disponibile per il VideoSourceToken: " << videoSourceToken << std::endl;
    }
    } */
/*
    }
    check_response(soap);
     // Estrai e stampa i parametri di radiometria
    if (getRadiometryConfigResponse.Configuration) {
        const auto &config = getRadiometryConfigResponse.Configuration;
        std::cout << "ReflectedAmbientTemperature: " << config->RadiometryGlobalParameters->ReflectedAmbientTemperature << std::endl;
        std::cout << "Emissivity: " << config->RadiometryGlobalParameters->Emissivity << std::endl;
        std::cout << "DistanceToObject: " << config->RadiometryGlobalParameters->DistanceToObject << std::endl;
        if (config->RadiometryGlobalParameters->RelativeHumidity)
            std::cout << "RelativeHumidity: " << *config->RadiometryGlobalParameters->RelativeHumidity << std::endl;
        if (config->RadiometryGlobalParameters->AtmosphericTemperature)
            std::cout << "AtmosphericTemperature: " << *config->RadiometryGlobalParameters->AtmosphericTemperature << std::endl;
    } else {
        std::cerr << "Configurazione di radiometria non disponibile per il VideoSourceToken: " << videoSourceToken << std::endl;
    }

        //}
    }

   if (videoSourceToken.empty()) {
      std::cerr << "VideoSourceToken non trovato per il profilo: " << profileToken << std::endl;
      return;
    }

    // Inizializza il proxy per il servizio termico
    ThermalBindingProxy proxyThermal(soap);
    set_thermal_endpoint(&proxyThermal);



    // Richiedi la configurazione di radiometria
    _tth__GetRadiometryConfiguration getRadiometryConfig;
    _tth__GetRadiometryConfigurationResponse getRadiometryConfigResponse;
    getRadiometryConfig.VideoSourceToken =   videoSource -> token; // profileToken;
    set_credentials(soap);
    if (proxyThermal.GetRadiometryConfiguration(&getRadiometryConfig, getRadiometryConfigResponse)) {
        report_error(soap, __LINE__);
        return;
    }
    check_response(soap);

    // Estrai e stampa i parametri di radiometria
    if (getRadiometryConfigResponse.Configuration) {
        const auto &config = getRadiometryConfigResponse.Configuration;
        std::cout << "ReflectedAmbientTemperature: " << config->RadiometryGlobalParameters->ReflectedAmbientTemperature << std::endl;
        std::cout << "Emissivity: " << config->RadiometryGlobalParameters->Emissivity << std::endl;
        std::cout << "DistanceToObject: " << config->RadiometryGlobalParameters->DistanceToObject << std::endl;
        if (config->RadiometryGlobalParameters->RelativeHumidity)
            std::cout << "RelativeHumidity: " << *config->RadiometryGlobalParameters->RelativeHumidity << std::endl;
        if (config->RadiometryGlobalParameters->AtmosphericTemperature)
            std::cout << "AtmosphericTemperature: " << *config->RadiometryGlobalParameters->AtmosphericTemperature << std::endl;
    } else {
        std::cerr << "Configurazione di radiometria non disponibile per il VideoSourceToken: " << videoSourceToken << std::endl;
    }
    */
//}
//////////
/*
void subscribe_to_fire_event(struct soap *soap, const std::string &event_service_endpoint) {
    PullPointSubscriptionBindingProxy proxyEvent(soap);

    // Imposta l'endpoint del servizio eventi
    proxyEvent.soap_endpoint = event_service_endpoint.c_str();
    set_credentials(soap);

    // Crea la sottoscrizione per eventi
    _tev__CreatePullPointSubscription createSubscription;
    _tev__CreatePullPointSubscriptionResponse createSubscriptionResponse;
    if (proxyEvent.CreatePullPointSubscription(&createSubscription, createSubscriptionResponse) != SOAP_OK) {
        report_error(soap, __LINE__);
        return;
    }
    check_response(soap);

    // Verifica che l'endpoint di sottoscrizione sia valido
    if (createSubscriptionResponse.SubscriptionReference.Address.empty()) {
        std::cerr << "Errore: SubscriptionReference.Address non valido." << std::endl;
        return;
    }

    // Endpoint del punto di pull
    std::string pull_endpoint = createSubscriptionResponse.SubscriptionReference.Address;

    // Recupera messaggi
    PullPointSubscriptionBindingProxy proxyPullPoint(soap);
    proxyPullPoint.soap_endpoint = pull_endpoint.c_str();

    while (true) {
        _tev__PullMessages pullMessages;
        _tev__PullMessagesResponse pullMessagesResponse;

        // Configura la richiesta di messaggi
        pullMessages.Timeout = "PT10S"; // Timeout di 10 secondi
        pullMessages.MessageLimit = 10; // Massimo 10 messaggi

        // Effettua la richiesta di messaggi
        if (proxyPullPoint.PullMessages(&pullMessages, pullMessagesResponse) != SOAP_OK) {
            report_error(soap, __LINE__);
            break;
        }
        check_response(soap);

        // Analizza i messaggi ricevuti
        for (const auto &msg : pullMessagesResponse.wsnt__NotificationMessage) {
            if (msg && msg->Message.Key) {  // Usa il "." al posto di "->"
                std::string msgContent = *msg->Message.Key->name;  // Corretto l'accesso ai campi

                // Cerca l'evento "isFire"
                if (msgContent.find("isFire") != std::string::npos) {
                    std::cout << "Evento isFire rilevato: " << msgContent << std::endl;
                }
            }
        }

        // Aggiungi una pausa per evitare un loop infinito troppo veloce
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
*/


std::string createPullPointSubscription(struct soap* soap, const char* event_service_endpoint) {
    PullPointSubscriptionBindingProxy proxyEvent(soap);
    proxyEvent.soap_endpoint = event_service_endpoint;

    _tev__CreatePullPointSubscription request;
    _tev__CreatePullPointSubscriptionResponse response;

    wsnt__AbsoluteOrRelativeTimeType initialTerminationTime = "PT1H"; // 1 ora di durata (durata relativa)
    request.InitialTerminationTime = &initialTerminationTime;

    int ret = proxyEvent.CreatePullPointSubscription(&request, response);
    if (ret != SOAP_OK) {
        std::cerr << "Errore nella creazione della sottoscrizione" << std::endl;
        exit(EXIT_FAILURE);
    }

    return response.SubscriptionReference.Address;
}

// Funzione per inviare la richiesta PullMessages
void pullMessages(struct soap* soap, const std::string& endpoint) {
    PullPointSubscriptionBindingProxy proxyPullPoint(soap);
    proxyPullPoint.soap_endpoint = endpoint.c_str();

    _tev__PullMessages pullRequest;
    pullRequest.Timeout = "60"; // Timeout in secondi
    pullRequest.MessageLimit = 10; // Numero massimo di messaggi

    _tev__PullMessagesResponse pullResponse;
    int ret = proxyPullPoint.PullMessages(&pullRequest, pullResponse);
    if (ret != SOAP_OK) {
        std::cerr << "Errore nella richiesta PullMessages" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Elaborazione dei messaggi ricevuti
    for (const auto& message : pullResponse.wsnt__NotificationMessage) {
        std::cout << "Received message: " << message << std::endl;
    }
}

// Funzione per l'operazione di Unsubscribe
void unsubscribe(struct soap* soap, const std::string& endpoint) {
    PullPointSubscriptionBindingProxy proxyEvent(soap);
    proxyEvent.soap_endpoint = endpoint.c_str();

    _wsnt__Unsubscribe unsubscribeRequest;
    _wsnt__UnsubscribeResponse unsubscribeResponse;

    int ret = proxyEvent.Unsubscribe(&unsubscribeRequest, unsubscribeResponse);
    if (ret != SOAP_OK) {
        std::cerr << "Errore nella richiesta di Unsubscribe" << std::endl;
        exit(EXIT_FAILURE);
    }
}

//////////////////
int main()
{
 
  // make OpenSSL MT-safe with mutex
  //CRYPTO_thread_setup();

  // create a context with strict XML validation and exclusive XML canonicalization for WS-Security enabled
  struct soap *soap = soap_new1( SOAP_XML_CANONICAL);
  //soap->fignore = skip_unknown;
  soap->connect_timeout = soap->recv_timeout = soap->send_timeout = 10; // 10 sec
  soap_register_plugin(soap, soap_wsse);

  // enable https connections with server certificate verification using cacerts.pem
  //if (soap_ssl_client_context(soap, SOAP_SSL_SKIP_HOST_CHECK, NULL, NULL, "cacerts.pem", NULL, NULL))
  if (soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL, NULL, NULL))
    report_error(soap, __LINE__);

  // create the proxies to access the ONVIF service API at HOSTNAME
  DeviceBindingProxy proxyDevice(soap);
  MediaBindingProxy proxyMedia(soap);

  // get device info and print
  //proxyDevice.soap_endpoint = HOSTNAME;
  set_device_endpoint(&proxyDevice);
  _tds__GetDeviceInformation GetDeviceInformation;
  _tds__GetDeviceInformationResponse GetDeviceInformationResponse;
  set_credentials(soap);
  if (proxyDevice.GetDeviceInformation(&GetDeviceInformation, GetDeviceInformationResponse))
    report_error(soap, __LINE__);
  check_response(soap);
  std::cout << "Manufacturer:    " << GetDeviceInformationResponse.Manufacturer << std::endl;
  std::cout << "Model:           " << GetDeviceInformationResponse.Model << std::endl;
  std::cout << "FirmwareVersion: " << GetDeviceInformationResponse.FirmwareVersion << std::endl;
  std::cout << "SerialNumber:    " << GetDeviceInformationResponse.SerialNumber << std::endl;
  std::cout << "HardwareId:      " << GetDeviceInformationResponse.HardwareId << std::endl;



    
  // get device capabilities and print media
  _tds__GetCapabilities GetCapabilities;
  _tds__GetCapabilitiesResponse GetCapabilitiesResponse;
  set_credentials(soap);
  if (proxyDevice.GetCapabilities(&GetCapabilities, GetCapabilitiesResponse))
    report_error(soap, __LINE__);
  check_response(soap);
  if (!GetCapabilitiesResponse.Capabilities || !GetCapabilitiesResponse.Capabilities->Media)
  {
    std::cerr << "Missing device capabilities info" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cout << "XAddr:        " << GetCapabilitiesResponse.Capabilities->Media->XAddr << std::endl;
  //subscribe_to_fire_event(soap, event_service_endpoint);


  //altre capacità da controllare
  /*
  std::cout << "XAddr imaging:        " << GetCapabilitiesResponse.Capabilities->Imaging->XAddr << std::endl;
  std::cout << "XAddr analityst:        " << GetCapabilitiesResponse.Capabilities->Analytics->XAddr << std::endl;
  std::cout << "analytics rulesupport:        " << GetCapabilitiesResponse.Capabilities->Analytics->RuleSupport << std::endl;
  std::cout << "analityst module support:        " << GetCapabilitiesResponse.Capabilities->Analytics->AnalyticsModuleSupport << std::endl;
  */

  if (GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities) {
    if (GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTPMulticast)
      std::cout << "RTPMulticast: " << (*GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTPMulticast ? "yes" : "no") << std::endl;
    else 
      std::cout << "RTPMulticast capability not available" << std::endl;

    if (GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTP_USCORETCP)
      std::cout << "RTP_TCP:      " << (*GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTP_USCORETCP ? "yes" : "no") << std::endl;
    else 
      std::cout << "RTP_TCP capability not available" << std::endl;
    if (GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP)
      std::cout << "RTP_RTSP_TCP: " << (*GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP ? "yes" : "no") << std::endl;
    else
      std::cout << "RTP_RTSP_TCP capability not available" << std::endl;
  } else {
  std::cout << "StreamingCapabilities not available" << std::endl;
}

  // set the Media proxy endpoint to XAddr
  proxyMedia.soap_endpoint = GetCapabilitiesResponse.Capabilities->Media->XAddr.c_str();

 // proxyMedia.soap_endpoint = "http://192.168.20.104/onvif/device_service";
  std::cout << "endpoint: " << proxyMedia.soap_endpoint << std::endl;

  // get device profiles
  _trt__GetProfiles GetProfiles;
  _trt__GetProfilesResponse GetProfilesResponse;
  set_credentials(soap);
  if (proxyMedia.GetProfiles(&GetProfiles, GetProfilesResponse))
    report_error(soap, __LINE__);
  check_response(soap);
  

  // for each profile get snapshot
  for (size_t i = 0; i < GetProfilesResponse.Profiles.size(); ++i)
  {

       // Ottieni lo stream URI direttamente nel main
   //proxyMedia.soap_endpoint = GetCapabilitiesResponse.Capabilities->Media->XAddr.c_str(); // Imposta l'endpoint media
   _trt__GetStreamUri GetStreamUri;
   _trt__GetStreamUriResponse GetStreamUriResponse;
   GetStreamUri.ProfileToken = GetProfilesResponse.Profiles[i]->token;
   set_credentials(soap);
        
   std::cout << "Media service endpoint: " << proxyMedia.soap_endpoint << " - " << GetProfilesResponse.Profiles[i]->token <<std::endl;

   if (proxyMedia.GetStreamUri(&GetStreamUri, GetStreamUriResponse)) {
       report_error(soap, __LINE__);
   }
   check_response(soap);

   if (GetStreamUriResponse.MediaUri) {
       std::cout << "Stream URI: " << GetStreamUriResponse.MediaUri->Uri << std::endl;
       
   } else {
       std::cerr << "Failed to retrieve stream URI" << std::endl;
   }
/*
   /////////////////////
  _tth__GetRadiometryConfiguration getRadiometryConfig;
    _tth__GetRadiometryConfigurationResponse getRadiometryConfigResponse;
   getRadiometryConfig.VideoSourceToken = GetProfilesResponse.Profiles[i]->token;
   set_credentials(soap);
        
        ThermalBindingProxy proxyThermal(soap);
    set_thermal_endpoint(&proxyThermal);

   std::cout << "sto nel test " << proxyThermal.soap_endpoint << " - " << GetProfilesResponse.Profiles[i]->token <<std::endl;

   /*if (proxyThermal.getRadiometryConfig(&getRadiometryConfig, getRadiometryConfigResponse)) {
       report_error(soap, __LINE__);
   }
   */
   /*
   check_response(soap);

   if (getRadiometryConfigResponse.Configuration) {
       std::cout << "test 2: " << getRadiometryConfigResponse.Configuration << std::endl;
       
   } else {
       std::cerr << "Failed to test" << std::endl;
   }
/////////////////////////////////////

   
*/
   
   
    // get snapshot URI for profile
    _trt__GetSnapshotUri GetSnapshotUri;
    _trt__GetSnapshotUriResponse GetSnapshotUriResponse;
    GetSnapshotUri.ProfileToken = GetProfilesResponse.Profiles[i]->token;
    set_credentials(soap);
    if (proxyMedia.GetSnapshotUri(&GetSnapshotUri, GetSnapshotUriResponse))
      report_error(soap, __LINE__);
    check_response(soap);
    std::cout << "Profile name: " << GetProfilesResponse.Profiles[i]->Name << std::endl;
    if (GetSnapshotUriResponse.MediaUri)
      save_snapshot(i, GetSnapshotUriResponse.MediaUri->Uri.c_str());

    //get_thermal_info(soap, GetProfilesResponse.Profiles[i]->token);

//    const char *device_service = "http://192.168.20.104/onvif/event_service"; // Imposta l'endpoint del servizio eventi


  // get_pullpoints(soap, device_service);
   std::cout<<"sono prima del pull"<<std::endl;
   
    
  }

   //struct soap* soap = soap_new();
   // std::string endpoint = createPullPointSubscription(soap, event_service_endpoint);

    // Autenticazione
    //struct soap* authenticatedSoap = authenticate(endpoint.c_str());

    // Invio della richiesta PullMessages
    //pullMessages(authenticatedSoap, endpoint);

    // Operazione di Unsubscribe
    //unsubscribe(authenticatedSoap, endpoint);


    

  

  // free all deserialized and managed data, we can still reuse the context and proxies after this
  soap_destroy(soap);
  soap_end(soap);

  // free the shared context, proxy classes must terminate as well after this
  soap_free(soap);

  

  // clean up OpenSSL mutex
  CRYPTO_thread_cleanup();

  return 0;
}



/******************************************************************************\
 *
 *	WS-Discovery event handlers must be defined, even when not used
 *
\******************************************************************************/

void wsdd_event_Hello(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, const char *EndpointReference, const char *Types, const char *Scopes, const char *MatchBy, const char *XAddrs, unsigned int MetadataVersion)
{ }

void wsdd_event_Bye(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, const char *EndpointReference, const char *Types, const char *Scopes, const char *MatchBy, const char *XAddrs, unsigned int *MetadataVersion)
{ }

soap_wsdd_mode wsdd_event_Probe(struct soap *soap, const char *MessageID, const char *ReplyTo, const char *Types, const char *Scopes, const char *MatchBy, struct wsdd__ProbeMatchesType *ProbeMatches)
{
  return SOAP_WSDD_ADHOC;
}

void wsdd_event_ProbeMatches(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, struct wsdd__ProbeMatchesType *ProbeMatches)
{ }

soap_wsdd_mode wsdd_event_Resolve(struct soap *soap, const char *MessageID, const char *ReplyTo, const char *EndpointReference, struct wsdd__ResolveMatchType *match)
{
  return SOAP_WSDD_ADHOC;
}

void wsdd_event_ResolveMatches(struct soap *soap, unsigned int InstanceId, const char * SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, struct wsdd__ResolveMatchType *match)
{ }

int SOAP_ENV__Fault(struct soap *soap, char *faultcode, char *faultstring, char *faultactor, struct SOAP_ENV__Detail *detail, struct SOAP_ENV__Code *SOAP_ENV__Code, struct SOAP_ENV__Reason *SOAP_ENV__Reason, char *SOAP_ENV__Node, char *SOAP_ENV__Role, struct SOAP_ENV__Detail *SOAP_ENV__Detail)
{
  // populate the fault struct from the operation arguments to print it
  soap_fault(soap);
  // SOAP 1.1
  soap->fault->faultcode = faultcode;
  soap->fault->faultstring = faultstring;
  soap->fault->faultactor = faultactor;
  soap->fault->detail = detail;
  // SOAP 1.2
  soap->fault->SOAP_ENV__Code = SOAP_ENV__Code;
  soap->fault->SOAP_ENV__Reason = SOAP_ENV__Reason;
  soap->fault->SOAP_ENV__Node = SOAP_ENV__Node;
  soap->fault->SOAP_ENV__Role = SOAP_ENV__Role;
  soap->fault->SOAP_ENV__Detail = SOAP_ENV__Detail;
  // set error
  soap->error = SOAP_FAULT;
  // handle or display the fault here with soap_stream_fault(soap, std::cerr);
  // return HTTP 202 Accepted
  return soap_send_empty_response(soap, SOAP_OK);
}

/******************************************************************************\
 *
 *	OpenSSL
 *
\******************************************************************************/

#ifdef WITH_OPENSSL

struct CRYPTO_dynlock_value
{ MUTEX_TYPE mutex;
};

static MUTEX_TYPE *mutex_buf;

/*static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line)
{ struct CRYPTO_dynlock_value *value;
  value = (struct CRYPTO_dynlock_value*)malloc(sizeof(struct CRYPTO_dynlock_value));
  if (value)
    MUTEX_SETUP(value->mutex);
  return value;
}*/

/*static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line)
{ if (mode & CRYPTO_LOCK)
    MUTEX_LOCK(l->mutex);
  else
    MUTEX_UNLOCK(l->mutex);
}
*/
/*static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line)
{ MUTEX_CLEANUP(l->mutex);
  free(l);
}
*/
void locking_function(int mode, int n, const char *file, int line)
{ if (mode & CRYPTO_LOCK)
    MUTEX_LOCK(mutex_buf[n]);
  else
    MUTEX_UNLOCK(mutex_buf[n]);
}

unsigned long id_function()
{ return (unsigned long)THREAD_ID;
}
/*
int CRYPTO_thread_setup()
{ int i;
  mutex_buf = (MUTEX_TYPE*)malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
  if (!mutex_buf)
    return SOAP_EOM;
  for (i = 0; i < CRYPTO_num_locks(); i++)
    MUTEX_SETUP(mutex_buf[i]);
  CRYPTO_set_id_callback(id_function);
  CRYPTO_set_locking_callback(locking_function);
  CRYPTO_set_dynlock_create_callback(dyn_create_function);
  CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
  CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
  return SOAP_OK;
}
*/

void CRYPTO_thread_cleanup()
{ int i;
  if (!mutex_buf)
    return;
  CRYPTO_set_id_callback(NULL);
  CRYPTO_set_locking_callback(NULL);
  CRYPTO_set_dynlock_create_callback(NULL);
  CRYPTO_set_dynlock_lock_callback(NULL);
  CRYPTO_set_dynlock_destroy_callback(NULL);
  for (i = 0; i < CRYPTO_num_locks(); i++)
    MUTEX_CLEANUP(mutex_buf[i]);
  free(mutex_buf);
  mutex_buf = NULL;
}

#else

/* OpenSSL not used */
/*
int CRYPTO_thread_setup()
{
  return SOAP_OK;
}
*/
void CRYPTO_thread_cleanup()
{ }

#endif




/*c++ -o ipcamera -Wall -std=c++11 -DWITH_OPENSSL -DWITH_DOM -DWITH_ZLIB -I. -I ~/gsoap-2.8/gsoap/plugin -I ~/gsoap-2.8/gsoap/custom -I ~/gsoap-2.8/gsoap/plugin main.cpp soapC.cpp wsddClient.cpp wsddServer.cpp soapAdvancedSecurityServiceBindingProxy.cpp soapDeviceBindingProxy.cpp soapDeviceIOBindingProxy.cpp soapImagingBindingProxy.cpp soapMediaBindingProxy.cpp soapPTZBindingProxy.cpp soapPullPointSubscriptionBindingProxy.cpp soapRemoteDiscoveryBindingProxy.cpp ~/gsoap-2.8/gsoap/stdsoap2.cpp ~/gsoap-2.8/gsoap/dom.cpp ~/gsoap-2.8/gsoap/plugin/smdevp.c ~/gsoap-2.8/gsoap/plugin/mecevp.c ~/gsoap-2.8/gsoap/plugin/wsaapi.c ~/gsoap-2.8/gsoap/plugin/wsseapi.c ~/gsoap-2.8/gsoap/plugin/wsddapi.c ~/gsoap-2.8/gsoap/plugin/httpda.c ~/gsoap-2.8/gsoap/custom/struct_timeval.c -lcrypto -lssl -lz -lpthread*/