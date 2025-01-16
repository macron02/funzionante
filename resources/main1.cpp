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
#include <csignal>
#include <atomic>
#include <iomanip>
#include <ctime>

#include <mutex>

#define USERNAME "admin"
#define PASSWORD "Password_01"
#define HOSTNAME "192.168.20.104"



std::atomic<bool> terminate_program(false);
// Profiletokenresponse

// Definisci la struttura Camera
struct xaddrs_t {
	std::string xaddr;  // Endpoint della telecamera
	std::string media_endpoint;
	std::vector<std::string> token_vector;
	std::string responsecreatesub;
};

std::vector<xaddrs_t> xaddrs_vector;

std::mutex xaddr_mutex;

std::vector<xaddrs_t> &get_xaddrs_vector() {
	return xaddrs_vector;
}

/******************************************************************************\
 *
 *	WS-Discovery event handlers must be defined, even when not used
 *
\******************************************************************************/

template <class T>
void printMatch(const T & match)
{
	//std::cout<< "TID: " << gettid()  << "===================================================================" << std::endl;
	//if (match.wsa5__EndpointReference.Address)
	//{
	//	std::cout<< "TID: " << gettid()  << "Endpoint:\t"<< match.wsa5__EndpointReference.Address << std::endl;
	//}
	//if (match.Types)
	//{
	//	std::cout<< "TID: " << gettid()  << "Types:\t\t"<< match.Types<< std::endl;
	//}
	//if (match.Scopes)
	//{
	//	if (match.Scopes->__item )
	//	{
	//		std::cout<< "TID: " << gettid()  << "Scopes:\t\t"<< match.Scopes->__item << std::endl;
	//	}
	//	if (match.Scopes->MatchBy)
	//	{
	//		std::cout<< "TID: " << gettid()  << "MatchBy:\t"<< match.Scopes->MatchBy << std::endl;
	//	}
	//}
	xaddr_mutex.lock();
	if (match.XAddrs) {

		for (auto & element : get_xaddrs_vector()) {
			if (!strcmp(element.xaddr.c_str(), match.XAddrs)) {
				return;
			}
		}
		//std::cout<< "TID: " << gettid()  << "XAddrs:\t\t"<< match.XAddrs << std::endl;
		xaddrs_t new_xaddr;
		new_xaddr.xaddr = match.XAddrs;
		get_xaddrs_vector().push_back(new_xaddr);
	}
	xaddr_mutex.unlock();

	//std::cout<< "TID: " << gettid()  << "MetadataVersion:\t\t" << match.MetadataVersion << std::endl;
	//std::cout<< "TID: " << gettid()  << "-------------------------------------------------------------------" << std::endl;

}



//static void set_device_endpoint(DeviceBindingProxy *dev)
//{
//	static char soap_endpoint[1024];
//	sprintf(soap_endpoint, "http://%s/onvif/device_service", HOSTNAME);
//
//	dev->soap_endpoint = soap_endpoint;
//}

//static void set_pull_endpoint(PullPointSubscriptionBindingProxy *dev)
//{
//	static char soap_endpoint[1024];
//	sprintf(soap_endpoint, "http://%s/onvif/device_service", HOSTNAME);
//
//	dev->soap_endpoint = soap_endpoint;
//
//}

//static void set_media_endpoint(MediaBindingProxy *dev)
//{
//	static char soap_endpoint[1024];
//	sprintf(soap_endpoint, "http://%s/onvif/device_service", HOSTNAME);
//
//	dev->soap_endpoint = soap_endpoint;
//}

//static void set_thermal_endpoint(ThermalBindingProxy *dev)
//{
//	static char soap_endpoint[1024];
//	sprintf(soap_endpoint, "http://%s/onvif/device_service", HOSTNAME);
//
//	dev->soap_endpoint = soap_endpoint;
//}


void wsdd_event_ProbeMatches(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, struct wsdd__ProbeMatchesType *matches)
{
	printf("wsdd_event_ProbeMatches tid:%s RelatesTo:%s nbMatch:%d\n", MessageID, RelatesTo, matches->__sizeProbeMatch);
	for (int i=0; i < matches->__sizeProbeMatch; ++i) {
		wsdd__ProbeMatchType& elt = matches->ProbeMatch[i];
		printMatch(elt);
	}
}

void wsdd_event_ResolveMatches(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, struct wsdd__ResolveMatchType *match)
{
	printf("wsdd_event_ResolveMatches tid:%s RelatesTo:%s\n", MessageID, RelatesTo);
	printMatch(*match);
}

void wsdd_event_Hello(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, const char *EndpointReference, const char *Types, const char *Scopes, const char *MatchBy, const char *XAddrs, unsigned int MetadataVersion)
{
}

void wsdd_event_Bye(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, const char *EndpointReference, const char *Types, const char *Scopes, const char *MatchBy, const char *XAddrs, unsigned int *MetadataVersion)
{
}

soap_wsdd_mode wsdd_event_Resolve(struct soap *soap, const char *MessageID, const char *ReplyTo, const char *EndpointReference, struct wsdd__ResolveMatchType *match)
{
	return SOAP_WSDD_ADHOC;
}

soap_wsdd_mode wsdd_event_Probe(struct soap *soap, const char *MessageID, const char *ReplyTo, const char *Types, const char *Scopes, const char *MatchBy, struct wsdd__ProbeMatchesType *matches)
{
	return SOAP_WSDD_ADHOC;
}
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

struct CRYPTO_dynlock_value {
	MUTEX_TYPE mutex;
};

static MUTEX_TYPE *mutex_buf;

// static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line) { struct CRYPTO_dynlock_value *value;
//   value = (struct CRYPTO_dynlock_value*)malloc(sizeof(struct CRYPTO_dynlock_value));
//   if (value)
//     MUTEX_SETUP(value->mutex);
//   return value;
// }

// static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line)
// { if (mode & CRYPTO_LOCK)
//     MUTEX_LOCK(l->mutex);
//   else
//     MUTEX_UNLOCK(l->mutex);
// }
//
// static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line)
// { MUTEX_CLEANUP(l->mutex);
//   free(l);
// }

void locking_function(int mode, int n, const char *file, int line)
{	if (mode & CRYPTO_LOCK)
		MUTEX_LOCK(mutex_buf[n]);
	else
		MUTEX_UNLOCK(mutex_buf[n]);
}

unsigned long id_function() {
	return (unsigned long)THREAD_ID;
}

int CRYPTO_thread_setup()
{	int i;
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


void CRYPTO_thread_cleanup() {
	int i;
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

int CRYPTO_thread_setup() {
	return SOAP_OK;
}

void CRYPTO_thread_cleanup() { }

#endif

/******************************************************************************\
 *
 *	START
 *
*******************************************************************************/
void report_error (struct soap *soap, int lineno)
{
	std::cerr << "-----------------------------------------" << std::endl;
	std::cerr << "Oops, something went wrong: error: " << soap->error << " at line: " << lineno << std::endl;
	soap_stream_fault(soap, std::cerr);
	std::cerr << "-----------------------------------------" << std::endl;
	SOAP_OK;
}

// to check if an ONVIF service response was signed with WS-Security (when enabled)
void check_response(struct soap *soap)
{

}


void set_credentials(struct soap *soap) {
	// Aggiunge il timestamp (necessario per WS-Security)
	soap_wsse_add_Timestamp(soap, "Time", 10);

	// Aggiunge le credenziali con digest
	if (soap_wsse_add_UsernameTokenDigest(soap, "Auth", USERNAME, PASSWORD))
		report_error(soap, __LINE__);
}


struct soap* authenticate(const char* endpoint) {
	// Creazione del contesto SOAP
	struct soap* soap = soap_new();
	if (!soap) {
		// std::cerr << "Errore nella creazione del contesto SOAP" << std::endl;
		return NULL;
	}

	// Registrazione del plugin HTTP Digest Authentication
	soap_register_plugin(soap, http_da);

	// Configurazione dei timeout
	soap->connect_timeout = soap->recv_timeout = soap->send_timeout = 10; // 10 secondi

	// Configurazione SSL (se necessario)
	if (soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL, NULL, NULL)) {
		// std::cerr << "Errore nella configurazione SSL" << std::endl;
		soap_destroy(soap);
		soap_end(soap);
		soap_free(soap);
		return NULL;
	}

	// Primo tentativo senza autenticazione
	if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap)) {
		//  std::cout<< "TID: " << gettid()  << "HTTP status: " << soap->status << " | Auth realm: "
		//         << (soap->authrealm ? soap->authrealm : "None") << std::endl;

		if (soap->status == 401 && soap->authrealm) {
			// Tentativo con Digest Authentication
			struct http_da_info info;
			//   std::cout<< "TID: " << gettid()  << "Tentativo con Digest Authentication..." << std::endl;
			soap_strdup(soap, soap->authrealm);
			http_da_save(soap, &info, soap->authrealm, USERNAME, PASSWORD);

			if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap)) {
				//std::cerr << "Errore con Digest Authentication..." << std::endl;
				http_da_release(soap, &info);
				soap_destroy(soap);
				soap_end(soap);
				soap_free(soap);
				return NULL;
			} else {
				//std::cout<< "TID: " << gettid()  << "Autenticazione con Digest Authentication riuscita!" << std::endl;
			}

			http_da_release(soap, &info);
		} else {
			// Tentativo con HTTP Basic Authentication
			// std::cout<< "TID: " << gettid()  << "Tentativo con HTTP Basic Authentication..." << std::endl;
			soap->userid = USERNAME;
			soap->passwd = PASSWORD;

			if (soap_GET(soap, endpoint, NULL) || soap_begin_recv(soap)) {
				// std::cerr << "HTTP Basic Authentication fallita." << std::endl;
				soap_destroy(soap);
				soap_end(soap);
				soap_free(soap);
				return NULL;
			} else {
				// std::cout<< "TID: " << gettid()  << "Autenticazione con HTTP Basic Authentication riuscita!" << std::endl;
			}
		}
	} else {
		//std::cout<< "TID: " << gettid()  << "Autenticazione iniziale riuscita!" << std::endl;
	}

	// Termina la ricezione per completare il contesto SOAP
	soap_end_recv(soap);

	// Restituisce il contesto autenticato
	return soap;
}

void set_device_date_time(struct soap* soap, const std::string& hostname) {
    // Creazione dei proxy per accedere all'API ONVIF
    DeviceBindingProxy proxyDevice(soap);

    // Impostazione dell'endpoint del dispositivo
    proxyDevice.soap_endpoint = hostname.c_str();

    // Variabili per la richiesta e la risposta
    _tds__SetSystemDateAndTime SetSystemDateAndTime;
    _tds__SetSystemDateAndTimeResponse SetSystemDateAndTimeResponse;

    // Impostazione delle credenziali
    set_credentials(soap);

    // Configurazione del tipo di data/ora (Manuale)
    SetSystemDateAndTime.DateTimeType = tt__SetDateTimeType__Manual;

    // Attivazione della gestione dell'ora legale
    SetSystemDateAndTime.DaylightSavings = true;

    // Recupero della data e ora correnti del sistema
    time_t now = time(0);
    struct tm* utcTime = gmtime(&now);

    // Configurazione dell'orario UTC
    SetSystemDateAndTime.UTCDateTime = soap_new_tt__DateTime(soap, -1);
    SetSystemDateAndTime.UTCDateTime->Time = soap_new_tt__Time(soap, -1);
    SetSystemDateAndTime.UTCDateTime->Time->Hour = utcTime->tm_hour;
    SetSystemDateAndTime.UTCDateTime->Time->Minute = utcTime->tm_min;
    SetSystemDateAndTime.UTCDateTime->Time->Second = utcTime->tm_sec;

    // Configurazione della data UTC
    SetSystemDateAndTime.UTCDateTime->Date = soap_new_tt__Date(soap, -1);
    SetSystemDateAndTime.UTCDateTime->Date->Year = utcTime->tm_year + 1900;
    SetSystemDateAndTime.UTCDateTime->Date->Month = utcTime->tm_mon + 1;
    SetSystemDateAndTime.UTCDateTime->Date->Day = utcTime->tm_mday;

    // Configurazione del fuso orario (esempio: "CET-1CEST,M3.5.0,M10.5.0/3")
    SetSystemDateAndTime.TimeZone = soap_new_tt__TimeZone(soap, -1);
    SetSystemDateAndTime.TimeZone->TZ = "CET-1CEST,M3.5.0,M10.5.0/3";

    // Invio della richiesta
    if (proxyDevice.SetSystemDateAndTime(&SetSystemDateAndTime, SetSystemDateAndTimeResponse)) {
        report_error(soap, __LINE__);
    }

    // Controllo della risposta
    check_response(soap);

    // Conferma dell'operazione
    std::cout << "TID: " << gettid() << " Data e ora impostate con successo!" << std::endl;
}


// Funzione per ottenere le informazioni del dispositivo
void get_device_info(struct soap* soap, const std::string& hostname) {
	// Creazione dei proxy per accedere all'API ONVIF
	DeviceBindingProxy proxyDevice(soap);

//std::cout<< "TID: " << gettid()  << "hostname " << hostname  << std::endl;
	// Impostazione dell'endpoint del dispositivo
	proxyDevice.soap_endpoint = hostname.c_str();  // Imposta l'endpoint da parametro

	// Variabili per la risposta del dispositivo
	_tds__GetDeviceInformation GetDeviceInformation;
	_tds__GetDeviceInformationResponse GetDeviceInformationResponse;

	// Impostazione delle credenziali
	set_credentials(soap);

	// Richiesta delle informazioni del dispositivo
	if (proxyDevice.GetDeviceInformation(&GetDeviceInformation, GetDeviceInformationResponse)) {
		report_error(soap, __LINE__);
	}

	// Controllo della risposta
	check_response(soap);

	// Stampa delle informazioni del dispositivo
	std::cout<< "TID: " << gettid()  << " Manufacturer:    " << GetDeviceInformationResponse.Manufacturer << std::endl;
	std::cout<< "TID: " << gettid()  << " Model:           " << GetDeviceInformationResponse.Model << std::endl;
	std::cout<< "TID: " << gettid()  << " FirmwareVersion: " << GetDeviceInformationResponse.FirmwareVersion << std::endl;
	std::cout<< "TID: " << gettid()  << " SerialNumber:    " << GetDeviceInformationResponse.SerialNumber << std::endl;
	std::cout<< "TID: " << gettid()  << " HardwareId:      " << GetDeviceInformationResponse.HardwareId << std::endl;
}


// Funzione per ottenere le capacitC  del dispositivo
std::string get_device_capabilities(struct soap* soap, const std::string& hostname) {
	DeviceBindingProxy proxyDevice(soap);
	MediaBindingProxy proxyMedia(soap);

	// Impostazione dell'endpoint del dispositivo
	proxyDevice.soap_endpoint = hostname.c_str();  // Imposta l'endpoint

	_tds__GetCapabilities GetCapabilities;
	_tds__GetCapabilitiesResponse GetCapabilitiesResponse;

	// Impostazione delle credenziali
	set_credentials(soap);

	// Richiesta delle capacitC  del dispositivo
	if (proxyDevice.GetCapabilities(&GetCapabilities, GetCapabilitiesResponse)) {
		report_error(soap, __LINE__);
	}

	check_response(soap);

	if (!GetCapabilitiesResponse.Capabilities || !GetCapabilitiesResponse.Capabilities->Media) {
		std::cerr << "Missing device capabilities info" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout<< "TID: " << gettid()  << " XAddr:        " << GetCapabilitiesResponse.Capabilities->Media->XAddr << std::endl;

	if (GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities) {
		if (GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTPMulticast) {
			std::cout<< "TID: " << gettid()  << " RTPMulticast: "
			         << (*GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTPMulticast ? "yes" : "no")
			         << std::endl;
		} else {
			std::cout<< "TID: " << gettid()  << " RTPMulticast capability not available" << std::endl;
		}

		if (GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTP_USCORETCP) {
			std::cout<< "TID: " << gettid()  << " RTP_TCP:      "
			         << (*GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTP_USCORETCP ? "yes" : "no")
			         << std::endl;
		} else {
			std::cout<< "TID: " << gettid()  << " RTP_TCP capability not available" << std::endl;
		}

		if (GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP) {
			std::cout<< "TID: " << gettid()  << " RTP_RTSP_TCP: "
			         << (*GetCapabilitiesResponse.Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP ? "yes" : "no")
			         << std::endl;
		} else {
			std::cout<< "TID: " << gettid()  << " RTP_RTSP_TCP capability not available" << std::endl;
		}
	} else {
		std::cout<< "TID: " << gettid()  << " StreamingCapabilities not available" << std::endl;
	}

	// Impostazione dell'endpoint del Media Service
	std::string media_endpoint = GetCapabilitiesResponse.Capabilities->Media->XAddr;

	bool found = false;

	xaddr_mutex.lock();
	for (auto &element : get_xaddrs_vector()) {
		if (hostname == element.xaddr) {
			element.media_endpoint = media_endpoint; // Aggiorna l'elemento esistente
			std::cout << "TID: " << gettid()
			          << " xaddr: " << element.xaddr
			          << " - media_endpoint: " << element.media_endpoint
			          << std::endl;
			found = true; // Segnala che l'elemento C( stato trovato e aggiornato
			break; // Esci dal ciclo una volta trovato
		}
	}
	xaddr_mutex.unlock();

// Aggiungi un nuovo elemento solo se non esiste giC 
	if (!found) {
		xaddrs_t new_xaddr;
		new_xaddr.xaddr = hostname;
		new_xaddr.media_endpoint = media_endpoint;
		xaddr_mutex.lock();
		get_xaddrs_vector().push_back(new_xaddr); // Aggiungi al vettore
		xaddr_mutex.unlock();
		//std::cout << "TID: " << gettid() << " Media service endpoint: " << media_endpoint  << std::endl;
	}

	return media_endpoint;
// Restituiamo l'endpoint del Media Service
}
////////////////////// mi devo salvare questo di sopra e passarlo sotto
// Funzione per ottenere i profili del dispositivo
void get_device_profiles(struct soap* soap, const std::string& xaddr) {
	MediaBindingProxy proxyMedia(soap);
	xaddr_mutex.lock();
	for (auto & element : get_xaddrs_vector()) {
		if (xaddr == element.xaddr) {
			// std::cout << "TID: " << gettid() << "Assigning media_endpoint: " << element.media_endpoint << std::endl;
			proxyMedia.soap_endpoint = element.media_endpoint.c_str();
			break;
		}
	}
	xaddr_mutex.unlock();
	_trt__GetProfiles GetProfiles;
	_trt__GetProfilesResponse GetProfilesResponse;

	// Impostazione delle credenziali
	set_credentials(soap);

	// Richiesta dei profili media
	if (proxyMedia.GetProfiles(&GetProfiles, GetProfilesResponse)) {
		report_error(soap, __LINE__);
	}

	check_response(soap);

	// Per ogni profilo, otteniamo lo Stream URI
	for (size_t i = 0; i < GetProfilesResponse.Profiles.size(); ++i) {
		_trt__GetStreamUri GetStreamUri;
		_trt__GetStreamUriResponse GetStreamUriResponse;
		GetStreamUri.ProfileToken = GetProfilesResponse.Profiles[i]->token;

		// Impostazione delle credenziali per il media
		set_credentials(soap);

		//std::cout << "TID: " << gettid() << "Media service endpoint: " << proxyMedia.soap_endpoint
		//          << " - Profile Token: " << GetProfilesResponse.Profiles[i]->token << std::endl;

		if (proxyMedia.GetStreamUri(&GetStreamUri, GetStreamUriResponse)) {
			report_error(soap, __LINE__);
		}

		check_response(soap);

		// Stampa dello Stream URI
		if (GetStreamUriResponse.MediaUri) {
			std::cout << "TID: " << gettid() << "Stream URI: " << GetStreamUriResponse.MediaUri->Uri << std::endl;
		} else {
			std::cerr << "Failed to retrieve stream URI" << std::endl;
		}
	}

	if (!GetProfilesResponse.Profiles.empty() && GetProfilesResponse.Profiles[0] != nullptr) {
		for (size_t i = 0; i < GetProfilesResponse.Profiles.size(); ++i) {
			//std::cout << "TID: " << gettid() << " Found Profile token: " << GetProfilesResponse.Profiles[i]->token << std::endl;
			xaddr_mutex.lock();
			for (auto &item : get_xaddrs_vector()) {
				if (xaddr == item.xaddr) {
					item.token_vector.push_back(GetProfilesResponse.Profiles[i]->token);

					//std::cout << "TID: " << gettid() << " Updated xaddr: " << xaddr << " - Saved Profile Token: " << item.token_vector[i] << std::endl;
					break;
				}
			}
			xaddr_mutex.unlock();
		}
	} else {
		std::cerr << "No profiles available." << std::endl;
		xaddr_mutex.unlock();
	}
}

void save_snapshot(struct soap* soap, const std::string& xaddr) {
	//std::cout << "snapshot funzione" << std::endl;
	MediaBindingProxy proxyMedia(soap);
	xaddr_mutex.lock();
	for (auto & element : get_xaddrs_vector()) {
		if (xaddr == element.xaddr) {

			//std::cout<< "TID: " << gettid()  << "Assigning media_endpoint: "<< element.media_endpoint << std::endl;
			proxyMedia.soap_endpoint = element.media_endpoint.c_str();
			break;
		}
	}

	std::vector<std::string> Profiletokenresponse;

	for (auto & element : get_xaddrs_vector()) {
		if (xaddr == element.xaddr) {
			// std::cout<< "TID: " << gettid()  << "Assigning media_endpoint: "<< element.media_endpoint << std::endl;
			// size_t i = 0; i < GetProfilesResponse.Profiles.size(); ++i
			for (const auto &token : element.token_vector) {
				Profiletokenresponse.push_back(token);
			}
		}
	}
	if (Profiletokenresponse.empty()) {
		std::cerr << "No profiles available to take a snapshot." << std::endl;
		xaddr_mutex.unlock();
		return;
	}

	xaddr_mutex.unlock();


	// Usa il primo profilo disponibile per lo snapshot
	// std::string profile_token = GetProfilesResponse.Profiles[0]->token;

	for (const auto& profileToken : Profiletokenresponse) {
		_trt__GetSnapshotUri GetSnapshotUri;
		_trt__GetSnapshotUriResponse GetSnapshotUriResponse;

		// Assegna il token corrente al profilo
		GetSnapshotUri.ProfileToken = profileToken;
		set_credentials(soap);

		//std::cout << "Snapshot request for profile token: " << profileToken << std::endl;
		//std::cout << "Using endpoint: " << proxyMedia.soap_endpoint << std::endl;


		// Richiesta dello Snapshot URI
		if (proxyMedia.GetSnapshotUri(&GetSnapshotUri, GetSnapshotUriResponse)) {
			report_error(soap, __LINE__);
			continue;
		}

		check_response(soap);

		// Verifica e salva lo Snapshot URI
		if (GetSnapshotUriResponse.MediaUri) {
			std::string snapshot_uri = GetSnapshotUriResponse.MediaUri->Uri;
			//std::cout<< "TID: " << gettid()  << "Snapshot URI: " << snapshot_uri << std::endl;

			struct soap* soap = authenticate(snapshot_uri.c_str());

			static int counter = 0;  // Variabile statica per mantenere il valore tra le chiamate
			counter++;  // Incrementa il contatore ad ogni snapshot

			// Genera un nome unico per il file (aggiungi il contatore al nome del file)
			std::string filename = "snapshot_" + std::to_string(counter) + ".jpg";


			FILE* fd = fopen(filename.c_str(), "wb");
			if (!fd) {
				std::cerr << "Cannot open " << filename << " for writing." << std::endl;
				return;
			}

			std::cout<< "TID: " << gettid()  << "Retrieving " << filename << " from " << snapshot_uri << std::endl;

			size_t imagelen;
			char* image = soap_http_get_body(soap, &imagelen);
			if (!image) {
				//std::cerr << "Error retrieving HTTP body." << std::endl;
				report_error(soap, __LINE__);
				fclose(fd);
				continue;
			}

			fwrite(image, 1, imagelen, fd);
			fclose(fd);
		} else {
			std::cerr << "Failed to retrieve snapshot URI." << std::endl;
		}

	}

//    soap_destroy(soap);
//  soap_end(soap);
//
//  // free the shared context, proxy classes must terminate as well after this
//  soap_free(soap);


}


int create_pull_point_subscription(soap *soap, const std::string& xaddr) {


	PullPointSubscriptionBindingProxy proxyEvent(soap);
	xaddr_mutex.lock();
	for (auto & element : get_xaddrs_vector()) {
		if (xaddr == element.xaddr) {
			//std::cout<< "TID: " << gettid()  << "Assigning url: "<< element.xaddr << std::endl;
			proxyEvent.soap_endpoint = element.xaddr.c_str();
			break;
		}
	}
	xaddr_mutex.unlock();

	// proxyEvent.soap_endpoint = url.c_str();
	//std::cout<< "TID: " << gettid()  << "url tentativo: " << xaddr << std::endl;

	soap->connect_timeout = soap->recv_timeout = soap->send_timeout = 10; // 10 sec
	soap_register_plugin(soap, soap_wsse);
	soap_register_plugin(soap, http_da);


	set_credentials(soap);

	_tev__CreatePullPointSubscription request;
	_tev__CreatePullPointSubscriptionResponse response;
	wsnt__AbsoluteOrRelativeTimeType initialTerminationTime = "PT1H"; // 1 ora di durata (durata relativa)

	request.InitialTerminationTime = &initialTerminationTime;

	//  soap->connect_timeout = soap->recv_timeout = soap->send_timeout = 10; // 10 sec
	//  soap_register_plugin(soap, soap_wsse);
	//  soap_register_plugin(soap, http_da);


	// Esegui la richiesta di creazione della sottoscrizione
	int ret = proxyEvent.CreatePullPointSubscription(&request, response);
	if (ret != SOAP_OK) {
		if (proxyEvent.soap->status == 401 && proxyEvent.soap->authrealm) {
			//std::cerr << "TID: " << gettid() << "Non autorizzato nella createpull. Tentativo con Digest Authentication..." << std::endl;

			// Configurazione per Digest Authentication
			struct http_da_info info;
			soap_strdup(proxyEvent.soap, proxyEvent.soap->authrealm);
			http_da_save(proxyEvent.soap, &info, proxyEvent.soap->authrealm, USERNAME, PASSWORD);

			// Tentativo di inviare nuovamente la richiesta con Digest Authentication
			ret = proxyEvent.CreatePullPointSubscription(&request, response);
			if (ret != SOAP_OK) {
				// std::cerr << "TID: " << gettid() << "Errore anche con Digest Authentication nella createpull." << std::endl;
				http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
				report_error(proxyEvent.soap, __LINE__);
				return ret;
			} else {
				//std::cout<< "TID: " << gettid()  << "Autenticazione con Digest Authentication riuscita! nella createpull" << std::endl;
			}

			http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
		} else {
			// std::cerr << "Errore nella richiesta createsubscribe (codice: " << ret << ")." << std::endl;
			report_error(proxyEvent.soap, __LINE__);
			return ret;
		}
	} else {
		//std::cout<< "TID: " << gettid()  << "Richiesta Createsubscribe completata con successo!" << std::endl;
	}

	// soap_destroy(soap);
	// soap_end(soap);
	// soap_free(soap);
	//std::cout << "TID: " << gettid() << "SubscriptionReference Address test da controllare: " << response.SubscriptionReference.Address << std::endl;
	std::string responsecreatesub = response.SubscriptionReference.Address;
	xaddr_mutex.lock();
	for (auto& element : get_xaddrs_vector()) {
		if (xaddr == element.xaddr) {
			element.responsecreatesub = responsecreatesub;
			//std::cout<< "TID: " << gettid()  << "Updated xaddr: " << element.xaddr
			//         << " - response createsub: " << element.responsecreatesub << std::endl;
			break;
		}
	}
	xaddr_mutex.unlock();
	return SOAP_OK;


}
int pull_messages(soap *soap, const std::string& xaddr) {

	PullPointSubscriptionBindingProxy proxyEvent(soap);



	//for (auto & element : xaddrs_vector) {
	//    if (xaddr == element.xaddr) {
	//        std::cout<< "TID: " << gettid()  << "Assigning response create sub: "<< element.responsecreatesub << std::endl;
	//        proxyEvent.soap_endpoint = element.responsecreatesub.c_str();
	//        break;
	//    }
	//}

	//proxyEvent.soap_endpoint = response.SubscriptionReference.Address;

	_tev__PullMessages pullRequest;
	pullRequest.Timeout = "PT1H"; // Timeout in secondi
	pullRequest.MessageLimit = 100; // Numero massimo di messaggi
	xaddr_mutex.lock();
	for (auto & element : get_xaddrs_vector()) {
		if (xaddr == element.xaddr) {
			//std::cout<< "TID: " << gettid()  << "Assigning url nel pull: "<< element.responsecreatesub << std::endl;
			proxyEvent.soap_endpoint = element.responsecreatesub.c_str();

		}
	}
	xaddr_mutex.unlock();
	// proxyEvent.soap_endpoint = "http://192.168.20.104/onvif/event_service";
	// std::cout << "TID: " << gettid() << "proxyEvent.soap_endpoint: " << proxyEvent.soap_endpoint << std::endl;

	_tev__PullMessagesResponse pullResponse;
	int ret = proxyEvent.PullMessages(&pullRequest, pullResponse);

	if (ret != SOAP_OK) {


		if (proxyEvent.soap->status == 401 && proxyEvent.soap->authrealm) {
			//std::cerr <<  "TID: " << gettid() << "Non autorizzato. Tentativo con Digest Authentication nella pullmessage..." << std::endl;

			// Configurazione per Digest Authentication
			struct http_da_info info;
			soap_strdup(proxyEvent.soap, proxyEvent.soap->authrealm);
			http_da_save(proxyEvent.soap, &info, proxyEvent.soap->authrealm, USERNAME, PASSWORD);

			//pullRequest.Timeout = "PT1H"; // Timeout in secondi
			//pullRequest.MessageLimit = 100; // Numero massimo di messaggi



			// Tentativo di inviare nuovamente la richiesta con Digest Authentication
			ret = proxyEvent.PullMessages(&pullRequest, pullResponse);
			if (ret != SOAP_OK) {
				//std::cerr << "TID: " << gettid() << "Errore anche con Digest Authentication nella pullmessage." << std::endl;
				http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
				report_error(proxyEvent.soap, __LINE__);
				return ret;
			} else {
				//std::cout<< "TID: " << gettid()  << "Autenticazione con Digest Authentication riuscita! pullmessage" << std::endl;
			}

			http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
		} else {
			//std::cerr << "Errore nella richiesta PullMessages (codice: " << ret << ")." << std::endl;
			report_error(proxyEvent.soap, __LINE__);
			return ret;
		}
	} else {
		// std::cout<< "TID: " << gettid()  << "Richiesta PullMessages completata con successo!" << std::endl;
	}


	std::cout<< "TID: " << gettid()  << " current time: " << pullResponse.CurrentTime << std::endl;
	std::cout<< "TID: " << gettid()  << " termination time: " << pullResponse.TerminationTime << std::endl;

	for (auto item : pullResponse.wsnt__NotificationMessage) {
		if (item->Topic && item->Topic->__any) {
			std::string topic = item->Topic->__any;
			if (topic == "tns1:RuleEngine/VideoSource/FireAlarm") {
				// Ottieni l'orario corrente
				auto now = std::chrono::system_clock::now();
				std::time_t now_time = std::chrono::system_clock::to_time_t(now);
				std::cout<< "TID: " << gettid()  << " Fire alarm detected at " << std::ctime(&now_time) << std::endl;
				return 50;
			}
		}
	}

	// Attendi un po' prima di inviare la prossima richiesta
	std::this_thread::sleep_for(std::chrono::seconds(10));
	return SOAP_OK;
}

int unsubscribe_from_pull_point(soap *soap, const std::string& xaddr) {

	PullPointSubscriptionBindingProxy proxyEvent(soap);

	_wsnt__Unsubscribe unsubscribeRequest;
	_wsnt__UnsubscribeResponse unsubscribeResponse;

	xaddr_mutex.lock();
	for (auto & element : get_xaddrs_vector()) {
		if (xaddr == element.xaddr) {
			// std::cout<< "TID: " << gettid()  << "Assigning url nel unsub: "<< element.responsecreatesub << std::endl;
			proxyEvent.soap_endpoint = element.responsecreatesub.c_str();

		}
	}
	xaddr_mutex.unlock();


	int ret = proxyEvent.Unsubscribe(&unsubscribeRequest, unsubscribeResponse);
	if (ret != SOAP_OK) {
		if (proxyEvent.soap->status == 401 && proxyEvent.soap->authrealm) {
			//std::cerr <<  "TID: " << gettid() << " Non autorizzato nell unsub. Tentativo con Digest Authentication..." << std::endl;

			// Configurazione per Digest Authentication
			struct http_da_info info;
			soap_strdup(proxyEvent.soap, proxyEvent.soap->authrealm);
			http_da_save(proxyEvent.soap, &info, proxyEvent.soap->authrealm, USERNAME, PASSWORD);

			// Tentativo di inviare nuovamente la richiesta con Digest Authentication
			ret = proxyEvent.Unsubscribe(&unsubscribeRequest, unsubscribeResponse);
			if (ret != SOAP_OK) {
				// std::cerr <<   "TID: " << gettid() << " Errore anche con Digest Authentication.nell unsub" << std::endl;
				http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
				report_error(proxyEvent.soap, __LINE__);
				return ret;
			} else {
				//  std::cout<< "TID: " << gettid()  << "Autenticazione con Digest Authentication riuscita! nell unsub" << std::endl;
			}

			http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
		} else {
			//  std::cerr << "Errore nella richiesta unsubscribe (codice: " << ret << ")." << std::endl;
			report_error(proxyEvent.soap, __LINE__);
			return ret;
		}
	} else {
		std::cout<< "TID: " << gettid()  << " Richiesta Unsubscribe completata con successo! nell unsub" << std::endl;
	}

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return SOAP_OK;
}

void set_video_encoder_configuration(struct soap* soap, const std::string& xaddr, const int width, const int height) {
	std::cout<< "TID: "<< gettid() << "sono qui1" << std::endl;
    MediaBindingProxy proxyMedia(soap);
    xaddr_mutex.lock();
    for (auto& element : get_xaddrs_vector()) {
        if (xaddr == element.xaddr) {
            proxyMedia.soap_endpoint = element.media_endpoint.c_str();
            break;
        }
    }

    std::vector<std::string> Profiletokenresponse;
    for (auto& element : get_xaddrs_vector()) {
        if (xaddr == element.xaddr) {
            for (const auto& token : element.token_vector) {
                Profiletokenresponse.push_back(token);
            }
        }
    }

    if (Profiletokenresponse.empty()) {
        std::cerr << "No profiles available to set video encoder configuration." << std::endl;
        xaddr_mutex.unlock();
        return;
    }

    xaddr_mutex.unlock();

    // Usa il primo profilo disponibile
    for (const auto& profileToken : Profiletokenresponse) {
        _trt__SetVideoEncoderConfiguration SetVideoEncoderConfiguration;
        _trt__SetVideoEncoderConfigurationResponse SetVideoEncoderConfigurationResponse;

        // Prepara la configurazione del video encoder
        tt__VideoEncoderConfiguration config;
        config.Encoding = tt__VideoEncoding__H264;  // Impostazione codec H.264 (modificabile)
        config.Quality = 10.0f;  // Imposta la qualità (da 0 a 100)

        // Imposta la risoluzione desiderata
        config.Resolution = new tt__VideoResolution();
        config.Resolution->Width = width;
        config.Resolution->Height = height;

        // Imposta la configurazione
        SetVideoEncoderConfiguration.Configuration = &config;
        SetVideoEncoderConfiguration.ForcePersistence = true;  // Assume che sia sempre true come suggerito

        set_credentials(soap);

        // Invia la richiesta
        if (proxyMedia.SetVideoEncoderConfiguration(&SetVideoEncoderConfiguration, SetVideoEncoderConfigurationResponse)) {
            report_error(soap, __LINE__);
            continue;
        }

        check_response(soap);

        // Verifica la risposta
        std::cout << "Video encoder configuration set successfully!" << std::endl;
    }

}





#define MULTICAST_GROUP ("239.255.255.250")
#define PORT (3702)


char  g_scopes[] = "onvif://www.onvif.org/name/IPCAM";
// Funzione per inviare una richiesta di Probe o Resolve
void send_probe_or_resolve(const std::string& url, const char* endpoint) {
	std::vector<std::string> xaddrs_list;
	const char *type = NULL;
	const char *scope = NULL;

	int res = 0;
	if (url.find("soap.udp:") == 0) {
		// std::cout<< "TID: " << gettid()  << "to multicast" << std::endl;

		// Crea un'istanza SOAP per UDP
		struct soap* serv = soap_new1(SOAP_IO_UDP);
		if (!soap_valid_socket(soap_bind(serv, NULL, 0, 1000))) {
			soap_print_fault(serv, stderr);
			exit(1);
		}

		// Chiamata Probe o Resolve
		if (strlen(endpoint) == 0) {
			res = soap_wsdd_Probe(serv,
			                      SOAP_WSDD_ADHOC,      // ModalitC 
			                      SOAP_WSDD_TO_TS,      // Destinazione a TS
			                      "soap.udp://239.255.255.250:3702",         // Indirizzo TS
			                      soap_wsa_rand_uuid(serv),                   // ID messaggio
			                      NULL,                 // ReplyTo
			                      type,
			                      scope,
			                      NULL);
		} else {
			// Invia richiesta di Resolve
			res = soap_wsdd_Resolve(serv,
			                        SOAP_WSDD_ADHOC,      // ModalitC 
			                        SOAP_WSDD_TO_TS,      // Destinazione a TS
			                        "soap.udp://239.255.255.250:3702",         // Indirizzo TS
			                        soap_wsa_rand_uuid(serv),                   // ID messaggio
			                        NULL,                 // ReplyTo
			                        endpoint);
		}

		if (res != SOAP_OK) {
			soap_print_fault(serv, stderr);
			return;
		}

		// Ascolta le risposte
		soap_wsdd_listen(serv, -1000000);
	} else {
		// std::cout<< "TID: " << gettid()  << "to proxy" << std::endl;

		struct soap* serv = soap_new();
		if (strlen(endpoint) == 0) {
			res = soap_wsdd_Probe(serv,
			                      SOAP_WSDD_MANAGED,      // ModalitC 
			                      SOAP_WSDD_TO_DP,      // Destinazione a Proxy
			                      url.c_str(),         // Indirizzo Proxy
			                      soap_wsa_rand_uuid(serv),                   // ID messaggio
			                      NULL,                 // ReplyTo
			                      type,
			                      scope,
			                      NULL);
		} else {
			// Invia richiesta di Resolve
			res = soap_wsdd_Resolve(serv,
			                        SOAP_WSDD_MANAGED,      // ModalitC 
			                        SOAP_WSDD_TO_DP,      // Destinazione a Proxy
			                        url.c_str(),         // Indirizzo Proxy
			                        soap_wsa_rand_uuid(serv),                   // ID messaggio
			                        NULL,                 // ReplyTo
			                        endpoint);
		}

		if (res != SOAP_OK) {
			soap_print_fault(serv, stderr);
			return;
		}
	}
}


void signal_handler(int signal) {
	if (signal == SIGINT) {
		std::cout << "\nSegnale ricevuto: Terminazione richiesta (Ctrl+C).\n";
		terminate_program = true;
	}
}


void handle_camera(const std::string& xaddr) {
	struct soap* soap = soap_new();
	// std::cout<< "TID: " << gettid()  << "TID: " << gettid() << std::endl;
	set_credentials(soap);


	get_device_info(soap, xaddr);
    set_device_date_time(soap, xaddr);
	std::cout<< "TID: "<< gettid() << "sono qui2" << std::endl;
	get_device_capabilities(soap, xaddr);
	std::cout<< "TID: "<< gettid() << "sono qui3" << std::endl;
	get_device_profiles(soap, xaddr);
	std::cout<< "TID: "<< gettid() << "sono qui" << std::endl;
	set_video_encoder_configuration(soap, xaddr, 1920, 1080);



	// PullPointSubscriptionBindingProxy proxyEvent;
	if (create_pull_point_subscription(soap, xaddr) != SOAP_OK) {
		
		// std::cerr << "TID: " << gettid() << " - Errore nella creazione della subscription per: " << xaddr << std::endl;
		return;
	}
	else {
		//  std::cout<< "TID: " << gettid()  << "createSubscription created for: " << xaddr << std::endl;
	}


	int msg_res = pull_messages(soap, xaddr);
	while (!terminate_program) {
		if (msg_res == SOAP_OK) {

			// std::cout<< "TID: " << gettid()  << "pulling message:" << std::endl;
			if (pull_messages(soap, xaddr)==50) {
				//std::cout<< "TID: " << gettid()  << "nell if1 " << std::endl; // Condizione per allarme incendio
				save_snapshot(soap, xaddr);
				//  get_device_profiles(soap, xaddr);
				// std::cout<< "TID: " << gettid()  << "nell if " << std::endl;

			}
		} else {
			//  std::cerr << "Errore nel ricevere i messaggi per la telecamera: " << xaddr << std::endl;
		}
		std::cout<< "TID: " << gettid()  << " Waiting for 1 second..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	std::cout << "Eseguo l'unsubscribe prima della terminazione..." << std::endl;
	unsubscribe_from_pull_point(soap, xaddr);

	std::cout << "Terminazione completata.\n";
}

int main() {
	std::string url = "soap.udp://239.255.255.250:3702";
	const char* endpoint = "";
	std::signal(SIGINT, signal_handler);


	send_probe_or_resolve(url, endpoint);
	std::vector<std::thread> camera_threads;

	for (const auto& xaddr_element : get_xaddrs_vector()) {
		camera_threads.emplace_back(std::thread(handle_camera, xaddr_element.xaddr));
	}

	for (auto& t : camera_threads) {
		t.join();  // Aspetta che tutti i thread terminino
	}

	return 0;
}



/*int main () {
    const char * url = "http://192.168.20.104/onvif/device_service";
//    struct soap* soap = soap_new();

    CRYPTO_thread_setup();

    // create a context with strict XML validation and exclusive XML canonicalization for WS-Security enabled
    struct soap *soap = soap_new1( SOAP_XML_CANONICAL);
    //soap->fignore = skip_unknown;
    soap->connect_timeout = soap->recv_timeout = soap->send_timeout = 10; // 10 sec
    soap_register_plugin(soap, soap_wsse);
    soap_register_plugin(soap, http_da);

    if (soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL, NULL, NULL)){
        report_error(soap, __LINE__);
    }

    PullPointSubscriptionBindingProxy proxyEvent(soap);
    proxyEvent.soap_endpoint = url;

     set_credentials(soap);





    _tev__CreatePullPointSubscription           request;
    _tev__CreatePullPointSubscriptionResponse   response;
    wsnt__AbsoluteOrRelativeTimeType            initialTerminationTime = "PT1H"; // 1 ora di durata (durata relativa)




    request.InitialTerminationTime = &initialTerminationTime;
    //std::string endpoint = createPullPointSubscription(soap, url);

    int ret = proxyEvent.CreatePullPointSubscription(&request, response);
    if (ret != SOAP_OK) {
        std::cerr << "Errore nella creazione della sottoscrizione" << std::endl;
        report_error(soap, __LINE__);
    } else {
        std::cout<< "TID: " << gettid()  << "Creata la sottoscrizione: " << response.SubscriptionReference.Address << std::endl;
    }

    proxyEvent.soap_endpoint = response.SubscriptionReference.Address;



    while (true) {

    _tev__PullMessages pullRequest;
    pullRequest.Timeout = "PT1H"; // Timeout in secondi
    pullRequest.MessageLimit = 100; // Numero massimo di messaggi

    _tev__PullMessagesResponse pullResponse;
    int cit = proxyEvent.PullMessages(&pullRequest, pullResponse);
if (cit != SOAP_OK) {
    // Controllo dello stato HTTP
    if (proxyEvent.soap->status == 401 && proxyEvent.soap->authrealm) {
        std::cerr << "Non autorizzato. Tentativo con Digest Authentication..." << std::endl;

        // Configurazione per Digest Authentication
        struct http_da_info info;
        soap_strdup(proxyEvent.soap, proxyEvent.soap->authrealm);
      //  std::cout<< "TID: " << gettid()  << "authrealm: " << proxyEvent.soap->authrealm<< std::endl;// Copia del realm
        http_da_save(proxyEvent.soap, &info, proxyEvent.soap->authrealm, USERNAME, PASSWORD);
       // std::cout<< "TID: " << gettid()  << "authrealm: " << proxyEvent.soap->authrealm<< std::endl;

        // Tentativo di inviare nuovamente la richiesta con Digest Authentication
        cit = proxyEvent.PullMessages(&pullRequest, pullResponse);
        if (cit != SOAP_OK) {
            std::cerr << "Errore anche con Digest Authentication." << std::endl;
            http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
            report_error(proxyEvent.soap, __LINE__);
            exit(EXIT_FAILURE);
        } else {
            std::cout<< "TID: " << gettid()  << "Autenticazione con Digest Authentication riuscita!" << std::endl;
        }

        http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
    } else {
        std::cerr << "Errore nella richiesta PullMessages (codice: " << cit << ")." << std::endl;
        report_error(proxyEvent.soap, __LINE__);
        exit(EXIT_FAILURE);
    }
} else {
    std::cout<< "TID: " << gettid()  << "Richiesta PullMessages completata con successo!" << std::endl;
}


std::cout<< "TID: " << gettid()  << "current time: " << pullResponse.CurrentTime << std::endl;
std::cout<< "TID: " << gettid()  << "termination time: " << pullResponse.TerminationTime << std::endl;
///////////////////////////////////////////////////////////////////////////////////////////////
for (auto item : pullResponse.wsnt__NotificationMessage) {
                if (item->Topic && item->Topic->__any) {
                    std::string topic = item->Topic->__any;
                    if (topic == "tns1:RuleEngine/VideoSource/FireAlarm") {
                        // Ottieni l'orario corrente
                        auto now = std::chrono::system_clock::now();
                        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
                        std::cout<< "TID: " << gettid()  << "Fire alarm detected at " << std::ctime(&now_time) << std::endl;
                    }
                }
            }


        // Attendi un po' prima di inviare la prossima richiesta
        std::this_thread::sleep_for(std::chrono::seconds(10));



    }

    // Cleanup
    soap_destroy(soap);
    soap_end(soap);
    soap_free(soap);

    return 0;
}




*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////7
/*for (auto item : pullResponse.wsnt__NotificationMessage ) {
   //  std::cout<< "TID: " << gettid()  << "received SubscriptionReference __any: " << item->SubscriptionReference->__any<< std::endl;
   //  std::cout<< "TID: " << gettid()  << "received Topic __any: " << item->Topic->__any << std::endl;
   //  std::cout<< "TID: " << gettid()  << "received ProducerReference __any: " << item->ProducerReference->__any << std::endl;
     // std::cout<< "TID: " << gettid()  << "received __any: " << item->__any << std::endl;
     //std::cout<< "TID: " << gettid()  << "received SubscriptionReference __anyAttribute: " << item->SubscriptionReference->__anyAttribute<< std::endl;
     //std::cout<< "TID: " << gettid()  <<"1"<< std::endl;
     //std::cout<< "TID: " << gettid()  << "received Topic __anyAttribute: " << item->Topic->__anyAttribute << std::endl;
     //std::cout<< "TID: " << gettid()  <<"2"<< std::endl;
     //std::cout<< "TID: " << gettid()  << "received ProducerReference __anyAttribute: " << item->ProducerReference->__anyAttribute << std::endl;
     //std::cout<< "TID: " << gettid()  <<"3"<< std::endl;

     std::cout<< "TID: " << gettid()  << "received SubscriptionReference : " << item->SubscriptionReference<< std::endl;
     std::cout<< "TID: " << gettid()  << "received Topic : " << item->Topic << std::endl;
     std::cout<< "TID: " << gettid()  << "received ProducerReference: " << item->ProducerReference<< std::endl;
   }

   for (auto item : pullResponse.wsnt__NotificationMessage) {
   if (item->SubscriptionReference) {
       std::cout<< "TID: " << gettid()  << "received SubscriptionReference : " << item->SubscriptionReference << std::endl;
       if (item->SubscriptionReference->__any) {
           std::cout<< "TID: " << gettid()  << "received SubscriptionReference __any: " << item->SubscriptionReference->__any << std::endl;
       }
       if (item->SubscriptionReference->__anyAttribute) {
           std::cout<< "TID: " << gettid()  << "received SubscriptionReference __anyAttribute: " << item->SubscriptionReference->__anyAttribute << std::endl;
       }
   } else {
       std::cout<< "TID: " << gettid()  << "SubscriptionReference is null" << std::endl;
   }

   if (item->Topic) {
       std::cout<< "TID: " << gettid()  << "received Topic : " << item->Topic << std::endl;
       if (item->Topic->__any) {
           std::cout<< "TID: " << gettid()  << "received Topic __any: " << item->Topic->__any << std::endl;
       }
       if (item->Topic->__anyAttribute) {
           std::cout<< "TID: " << gettid()  << "received Topic __anyAttribute: " << item->Topic->__anyAttribute << std::endl;
       }
   } else {
       std::cout<< "TID: " << gettid()  << "Topic is null" << std::endl;
   }

   if (item->ProducerReference) {
       std::cout<< "TID: " << gettid()  << "received ProducerReference: " << item->ProducerReference << std::endl;
       if (item->ProducerReference->__any) {
           std::cout<< "TID: " << gettid()  << "received ProducerReference __any: " << item->ProducerReference->__any << std::endl;
       }
       if (item->ProducerReference->__anyAttribute) {
           std::cout<< "TID: " << gettid()  << "received ProducerReference __anyAttribute: " << item->ProducerReference->__anyAttribute << std::endl;
       }
   } else {
       std::cout<< "TID: " << gettid()  << "ProducerReference is null" << std::endl;
   }

}
*/

//std::cout<< "TID: " << gettid()  << "received message" << pullResponse.wsnt__NotificationMessage << std::endl;


// Autenticazione
//struct soap* authenticatedSoap = authenticate(soap, endpoint.c_str());

// Invio della richiesta PullMessages
//pullMessages(authenticatedSoap, endpoint);
// Creazione del proxy per il servizio eventi
//PullPointSubscriptionBindingProxy proxyEvent(soap);


// Invio della richiesta di Unsubscribe

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
_wsnt__Unsubscribe unsubscribeRequest;
_wsnt__UnsubscribeResponse unsubscribeResponse;

int lol = proxyEvent.Unsubscribe(&unsubscribeRequest, unsubscribeResponse);
if (lol != SOAP_OK) {
// Controllo dello stato HTTP
if (proxyEvent.soap->status == 401 && proxyEvent.soap->authrealm) {
    std::cerr << "Non autorizzato. Tentativo con Digest Authentication..." << std::endl;

    // Configurazione per Digest Authentication
    struct http_da_info info;
    soap_strdup(proxyEvent.soap, proxyEvent.soap->authrealm);
  //  std::cout<< "TID: " << gettid()  << "authrealm: " << proxyEvent.soap->authrealm<< std::endl;// Copia del realm
    http_da_save(proxyEvent.soap, &info, proxyEvent.soap->authrealm, USERNAME, PASSWORD);
   // std::cout<< "TID: " << gettid()  << "authrealm: " << proxyEvent.soap->authrealm<< std::endl;

    // Tentativo di inviare nuovamente la richiesta con Digest Authentication
    lol = proxyEvent.Unsubscribe(&unsubscribeRequest, unsubscribeResponse);
    if (lol != SOAP_OK) {
        std::cerr << "Errore anche con Digest Authentication." << std::endl;
        http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
        report_error(proxyEvent.soap, __LINE__);
        exit(EXIT_FAILURE);
    } else {
        std::cout<< "TID: " << gettid()  << "Autenticazione con Digest Authentication riuscita!" << std::endl;
    }

    http_da_release(proxyEvent.soap, &info);  // Rilascia risorse
} else {
    std::cerr << "Errore nella richiesta unsubscribe (codice: " << lol << ")." << std::endl;
    report_error(proxyEvent.soap, __LINE__);
    exit(EXIT_FAILURE);
}
} else {
std::cout<< "TID: " << gettid()  << "Richiesta Unsubscribe completata con successo!" << std::endl;
}

soap_destroy(soap);
soap_end(soap);
soap_free(soap);

CRYPTO_thread_cleanup();

}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////7

