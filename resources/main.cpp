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



#define USERNAME "admin"
#define PASSWORD "Password_01"
#define HOSTNAME "192.168.20.104"

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
void save_snapshot(int i, const char *endpoint)
{
    char filename[32];
    (SOAP_SNPRINTF_SAFE(filename, 32), "image-%d.jpg", i);
    FILE *fd = fopen(filename, "wb");
    if (!fd)
    {
        std::cerr << "Cannot open " << filename << " for writing" << std::endl;
        exit(EXIT_FAILURE);
    }

    // create a temporary context to retrieve the image with HTTP GET
    struct soap *soap = soap_new();
    // soap_register_plugin(soap, soap_wsse); // Registrare WS-Security nuovo
    soap_register_plugin(soap, http_da);    // Registro plugin digest http autentication
    struct http_da_info info;
    
    soap->connect_timeout = soap->recv_timeout = soap->send_timeout = 10; // 10 sec

    if (soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL, NULL, NULL)){
        report_error(soap, __LINE__);
    }

    std::cout << "GET " << endpoint << std::endl;

    // First attempt with Digest Authentication
    if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap))
    {
        std::cout << "HTTP status: " << soap->status << " | Auth realm: " 
                  << (soap->authrealm ? soap->authrealm : "None") << std::endl;

        if (soap->status == 401 && soap->authrealm)
        {
            // Attempt with Digest Authentication
            std::cout << "Tentativo con Digest Authentication..." << std::endl;

            soap_strdup(soap, soap->authrealm);
            http_da_save(soap, &info, soap->authrealm, USERNAME, PASSWORD);

            if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap)) {
                std::cout << "Errore con Digest Authentication..." << std::endl;
            } else {
                std::cout << "Autenticazione con Digest Authentication riuscita!" << std::endl;
            }

            http_da_release(soap, &info); // release if auth is no longer needed
        }
        else
        {
            // Attempt with HTTP Basic Authentication
            std::cout << "Tentativo con HTTP Basic Authentication..." << std::endl;
            soap->userid = USERNAME;
            soap->passwd = PASSWORD;
            if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap))
            {
                std::cerr << "HTTP Basic Authentication fallita." << std::endl;
                report_error(soap, __LINE__);
            }
            else
            {
                std::cout << "Autenticazione con HTTP Basic Authentication riuscita!" << std::endl;
            }
        }
    }
    else
    {
        std::cout << "Autenticazione con Digest Authentication riuscita!" << std::endl;
    }

    std::cout << "Retrieving " << filename;
    if (soap->http_content)
        std::cout << " of type " << soap->http_content;
    std::cout << " from " << endpoint << std::endl;

    size_t imagelen;
    char *image = soap_http_get_body(soap, &imagelen); // NOTE: soap_http_get_body was renamed from soap_get_http_body in gSOAP 2.8.73
    if (!image) {
        std::cerr << "Errore nel recupero del corpo HTTP" << std::endl;
        report_error(soap, __LINE__);
    }
    soap_end_recv(soap);

    fwrite(image, 1, imagelen, fd);
    fclose(fd);

    //cleanup
    soap_destroy(soap);
    soap_end(soap);
    soap_free(soap);
}


/*
void get_pullpoints(struct soap *soap, const char *event_service_endpoint)
{
  ///////////
  PullPointSubscriptionBindingProxy proxyEvent(soap);

    // Imposta l'endpoint del servizio eventi
    proxyEvent.soap_endpoint = event_service_endpoint;
    set_credentials(soap);
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
*/

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
   */ /*
   check_response(soap);

   if (getRadiometryConfigResponse.Configuration) {
       std::cout << "test 2: " << getRadiometryConfigResponse.Configuration << std::endl;
       
   } else {
       std::cerr << "Failed to test" << getRadiometryConfigResponse.Configuration << std::endl;
   }
/////////////////////////////////////
*/ /*

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

/*
void createPullPointSubscription(struct soap* soap, const char *event_service_endpoint) {
    // Creazione del proxy per la sottoscrizione
    PullPointSubscriptionBindingProxy proxyEvent(soap);
    proxyEvent.soap_endpoint = event_service_endpoint;
    set_credentials(soap);

    std::cout<<"sono nella funzione"<<std::endl;

    // Creazione della richiesta
    _tev__CreatePullPointSubscription request;
    _tev__CreatePullPointSubscriptionResponse response;

    _wsnt__Unsubscribe                 Unsubscribe_req;	///< Input parameter
    _wsnt__UnsubscribeResponse         UnsubscribeResponse;	///< Output parameter

    proxyEvent.Unsubscribe(&Unsubscribe_req, UnsubscribeResponse);

  /////////////////////// int prova = proxyEvent.Unsubscribe(&Unsubscribe_req, UnsubscribeResponse);
    if(prova !=SOAP_OK){
      report_error(soap, __LINE__);
      return;
    }
//////////////////////////////


    wsnt__AbsoluteOrRelativeTimeType initialTerminationTime;

    initialTerminationTime  = "PT1H"; // 1 ora di durata (durata relativa)
    request.InitialTerminationTime = &initialTerminationTime;

    // Invio della richiesta
    int ret = proxyEvent.CreatePullPointSubscription(&request, response);
    if (ret != SOAP_OK) {
      //  soap_stream_fault(soap, std::cerr);
        report_error(soap, __LINE__);
        proxyEvent.Unsubscribe(&Unsubscribe_req, UnsubscribeResponse);
        return;
    }
    

    // Gestione della risposta
   // std::string address = response.SubscriptionReference.Address;
  std::cout << "Subscription Address: " << response.SubscriptionReference.Address << std::endl;

   // std::cout << "Metadata: " << response.SubscriptionReference.Metadata << std::endl;
    std::cout << "Termination Time: " << response.wsnt__TerminationTime << std::endl;
    std::cout << "Current Time: " << response.wsnt__CurrentTime << std::endl;

    // Creazione dell'UUID per il messaggio
    std::string uuid = std::string(soap_rand_uuid(soap, "urn:uuid:"));
    struct SOAP_ENV__Header header;
    header.wsa5__MessageID = (char*)uuid.c_str();
    header.wsa5__To = response.SubscriptionReference.Address;
    soap->header = &header;

    // Creazione della richiesta PullMessages
    _tev__PullMessages pullRequest;
    pullRequest.Timeout = "60"; // Timeout in secondi
    pullRequest.MessageLimit = 10; // Numero massimo di messaggi

    // Invio della richiesta PullMessages
    _tev__PullMessagesResponse pullResponse;
    ret = proxyEvent.PullMessages(&pullRequest, pullResponse);
    if (ret != SOAP_OK) {
   //     soap_stream_fault(soap, std::cerr);
        report_error(soap, __LINE__);
        proxyEvent.Unsubscribe(&Unsubscribe_req, UnsubscribeResponse);
        return;
    }

    // Elaborazione dei messaggi ricevuti
    for (const auto& message : pullResponse.wsnt__NotificationMessage) {
        std::cout << "Received message: " << message << std::endl;
    }

 //   _wsnt__Unsubscribe                 Unsubscribe_req;	///< Input parameter
 //   _wsnt__UnsubscribeResponse         UnsubscribeResponse;	///< Output parameter

    proxyEvent.Unsubscribe(&Unsubscribe_req, UnsubscribeResponse);
}
*/

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
   */ /*
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

    

   // get_pullpoints(soap, device_service);
   std::cout<<"sono prima del pull"<<std::endl;
   
    
  }
   const char *device_service = "http://192.168.20.104/onvif/event_service"; // Imposta l'endpoint del servizio eventi

  //createPullPointSubscription(soap, device_service);
  

  

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