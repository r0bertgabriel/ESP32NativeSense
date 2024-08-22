// bibliotecas base
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_event.h"

// conectividade wi-fi e mesh
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"

// tarefas do freeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// sockets TCP/IP
#include "lwip/sockets.h"
#include "lwip/sys.h"

static const char *MESH_TAG = "mesh_main";

static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;
static esp_netif_t *netif_sta = NULL;

#define RX_BUF_SIZE 256
#define TX_BUF_SIZE 128

void rx_task(void *param)
{
    uint8_t rx_buf[RX_BUF_SIZE];

    int flag = 0;
    mesh_addr_t sender;
    mesh_data_t data;

    data.data = rx_buf;
    data.size = RX_BUF_SIZE;

    while(1)
    {
        if(esp_mesh_recv(&sender, &data, portMAX_DELAY, &flag, NULL, 0) == ESP_OK){
            ESP_LOGI(MESH_TAG, "remetente: "MACSTR", msg: %s", MAC2STR(sender.addr), data.data);
        } else {
            ESP_LOGE(MESH_TAG, "Erro em inicializar tarefa RX.");
            break;
        }
    }
    vTaskDelete(NULL);
}

void tx_task(void *param)
{
    uint8_t tx_buf[TX_BUF_SIZE];

    static const uint8_t MAC_A[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0A};
    static const uint8_t MAC_B[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0B};
    static const uint8_t MAC_C[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C};

    mesh_addr_t A, B, C;
    memcpy((uint8_t *) &A, MAC_A, 6);
    memcpy((uint8_t *) &B, MAC_B, 6);
    memcpy((uint8_t *) &C, MAC_C, 6);

    static const char msg[] = "Hello!";

    memcpy((uint8_t *) &tx_buf, msg, strlen(msg));

    mesh_data_t data;

    data.data = tx_buf;
    data.size = TX_BUF_SIZE;

    while(1)
    {
        esp_mesh_send(&A, &data, MESH_DATA_P2P, NULL, 0);
        esp_mesh_send(&B, &data, MESH_DATA_P2P, NULL, 0);
        esp_mesh_send(&C, &data, MESH_DATA_P2P, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void rx_root_task(void *param) // receber pacotes internamente e enviar para rede externa
{
    uint8_t rx_buf[RX_BUF_SIZE];
    char *tx_buf;

    int flag = 0;
    mesh_addr_t sender;
    mesh_data_t data;

    data.data = rx_buf;
    data.size = RX_BUF_SIZE;

    // configurando endereco do servidor remoto
    struct sockaddr_in dest_addr = {
        .sin_addr.s_addr = inet_addr("10.42.0.1"), // <ip_do_app_da_rede_externa>
        .sin_family = AF_INET,
        .sin_port = htons(9999), // <porta_do_app_da_rede_externa_rx>
    };

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    while(1)
    {
        if(esp_mesh_recv(&sender, &data, portMAX_DELAY, &flag, NULL, 0) == ESP_OK){
            asprintf(&tx_buf, "remetente: "MACSTR", msg: %s\n", MAC2STR(sender.addr), data.data); // prepara buffer para envio
            ESP_LOGI(MESH_TAG, "%s", tx_buf); // imprime buffer nos logs
            sendto(sock, tx_buf, strlen(tx_buf), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)); // enviar buffer para servidor remoto
            free(tx_buf);
        } 
    }
    vTaskDelete(NULL);
}

void tx_root_task(void *param) // receber pacotes da rede externa e enviar internamente
{
    uint8_t rx_buf[RX_BUF_SIZE] = {0};

    // variavies para rotear pacotes
    const int node_num = 3; // quantidade max. de nodes na rede
    mesh_addr_t route_table[node_num]; // tabela de roteamento
    int route_table_size = 0; // tamanho da tabela de roteamento
    mesh_data_t data; // dados a serem enviados
    data.data = rx_buf;
    data.size = RX_BUF_SIZE;

    // variaveis para identificar a fonte do pacote recebido
    struct sockaddr_storage source_addr; 
    socklen_t socklen = sizeof(source_addr);

    // configurando endereco do servidor local
    struct sockaddr_in dest_addr = {
        .sin_addr.s_addr = inet_addr("0.0.0.0"),
        .sin_family = AF_INET,
        .sin_port = htons(9998),
    };

    // criar socket e vincula-lo ao ip:porta deste node
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    while(1)
    {
        recvfrom(sock, rx_buf, sizeof(rx_buf)-1, 0, (struct sockaddr *)&source_addr, &socklen); // esperar mensagem
        ESP_LOGI(MESH_TAG, "%s", rx_buf); // imprime buffer nos logs

        // preencher tabela de roteamento com endereco MAC dos nodes filhos
        esp_mesh_get_routing_table((mesh_addr_t *) &route_table, node_num*6, &route_table_size); 
        // iterar pela tabela e enviar dados obtidos de aplicacao externa para todos os nodes
        for (int i = 0; i < route_table_size; i++)
            esp_mesh_send(&route_table[i], &data, MESH_DATA_P2P, NULL, 0);

        memset(rx_buf, 0, sizeof(rx_buf)); // limpar buffer para novo uso
    }
    vTaskDelete(NULL);
}

void tx_child_task(void *param)
{
    uint8_t tx_buf[TX_BUF_SIZE] = {0};
    static const char msg[] = "Hello!";
    mesh_data_t data;

    memcpy((uint8_t *) &tx_buf, msg, strlen(msg));
    data.data = tx_buf;
    data.size = TX_BUF_SIZE;
    
    while(1)
    {
        esp_mesh_send(NULL, &data, 0, NULL, 0); // enviar mensagem para node principal
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    vTaskDelete(NULL);
}

void rx_child_task(void *param)
{
    uint8_t rx_buf[RX_BUF_SIZE];

    int flag = 0;
    mesh_addr_t sender;
    mesh_data_t data;

    data.data = rx_buf;
    data.size = RX_BUF_SIZE;

    while(1)
    {
        if(esp_mesh_recv(&sender, &data, portMAX_DELAY, &flag, NULL, 0) == ESP_OK)
            ESP_LOGI(MESH_TAG, "remetente: "MACSTR", msg: %s", MAC2STR(sender.addr), data.data);
    }
    vTaskDelete(NULL);
}

void check_health_task(void *param)
{
    wifi_ap_record_t ap;
    mesh_vote_t vote = {
        .percentage = 0.9,
        .is_rc_specified = false,
        .config = {
            .attempts = 15
        }
    };

    while(1)
    {
        // parar autoconfiguracao da rede antes de chamar API do Wi-Fi
        ESP_ERROR_CHECK(esp_mesh_set_self_organized(false, false)); 
        
        // coletar informacoes do access point (roteador)
        esp_wifi_sta_get_ap_info(&ap);
        
        // retornando autoconfiguracao
        ESP_ERROR_CHECK(esp_mesh_set_self_organized(true, false)); 
        
        ESP_LOGI(MESH_TAG, "rssi: %d", ap.rssi);
        if (ap.rssi < -10) {
            ESP_LOGI(MESH_TAG, "RSSI menor que -90, chamando nova eleicao!");
            esp_mesh_waive_root(&vote, MESH_VOTE_REASON_ROOT_INITIATED);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    vTaskDelete(NULL);
}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    mesh_addr_t id = {0,};
    static uint16_t last_layer = 0;

    switch (event_id) {
    case MESH_EVENT_STARTED: {
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED: {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 child_connected->aid,
                 MAC2STR(child_connected->mac));
    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED: {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 child_disconnected->aid,
                 MAC2STR(child_disconnected->mac));
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new, mesh_layer);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new, mesh_layer);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND: {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 no_parent->scan_times);
    }
    /* TODO handler for the failure */
    break;
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR", duty:%d",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr), connected->duty);
        last_layer = mesh_layer;
        // mesh_connected_indicator(mesh_layer);
        is_mesh_connected = true;
        if (esp_mesh_is_root()) {
            esp_netif_dhcpc_stop(netif_sta);
            esp_netif_dhcpc_start(netif_sta);
        }
        // esp_mesh_comm_p2p_start();
        if(esp_mesh_is_root()){
            xTaskCreate(check_health_task, "HEALTH_TASK", configMINIMAL_STACK_SIZE+1024, NULL, configMAX_PRIORITIES-3, NULL);
            xTaskCreate(tx_root_task, "TX_TASK", configMINIMAL_STACK_SIZE+2048, NULL, configMAX_PRIORITIES-3, NULL);
            xTaskCreate(rx_root_task, "RX_TASK", configMINIMAL_STACK_SIZE+2048, NULL, configMAX_PRIORITIES-3, NULL);
        } else {
            xTaskCreate(tx_child_task, "TX_TASK", configMINIMAL_STACK_SIZE+2048, NULL, configMAX_PRIORITIES-3, NULL);
            xTaskCreate(rx_child_task, "RX_TASK", configMINIMAL_STACK_SIZE+2048, NULL, configMAX_PRIORITIES-3, NULL);
        }
    }
    break;
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 disconnected->reason);
        is_mesh_connected = false;
        // mesh_disconnected_indicator();
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
        mesh_layer = layer_change->new_layer;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
        // mesh_connected_indicator(mesh_layer);
    }
    break;
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 MAC2STR(root_addr->addr));
    }
    break;
    case MESH_EVENT_VOTE_STARTED: {
        mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 vote_started->attempts,
                 vote_started->reason,
                 MAC2STR(vote_started->rc_addr.addr));
    }
    break;
    case MESH_EVENT_VOTE_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    }
    case MESH_EVENT_ROOT_SWITCH_REQ: {
        mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 switch_req->reason,
                 MAC2STR( switch_req->rc_addr.addr));
    }
    break;
    case MESH_EVENT_ROOT_SWITCH_ACK: {
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
    }
    break;
    case MESH_EVENT_TODS_STATE: {
        mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    }
    break;
    case MESH_EVENT_ROOT_FIXED: {
        mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 root_fixed->is_fixed ? "fixed" : "not fixed");
    }
    break;
    case MESH_EVENT_ROOT_ASKED_YIELD: {
        mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(root_conflict->addr),
                 root_conflict->rssi,
                 root_conflict->capacity);
    }
    break;
    case MESH_EVENT_CHANNEL_SWITCH: {
        mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    }
    break;
    case MESH_EVENT_SCAN_DONE: {
        mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 scan_done->number);
    }
    break;
    case MESH_EVENT_NETWORK_STATE: {
        mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 network_state->is_rootless);
    }
    break;
    case MESH_EVENT_STOP_RECONNECTION: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    }
    break;
    case MESH_EVENT_FIND_NETWORK: {
        mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 find_network->channel, MAC2STR(find_network->router_bssid));
    }
    break;
    case MESH_EVENT_ROUTER_SWITCH: {
        mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    }
    break;
    case MESH_EVENT_PS_PARENT_DUTY: {
        mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
    }
    break;
    case MESH_EVENT_PS_CHILD_DUTY: {
        mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, "MACSTR", duty:%d", ps_duty->child_connected.aid-1,
                MAC2STR(ps_duty->child_connected.mac), ps_duty->duty);
    }
    break;
    default:
        ESP_LOGI(MESH_TAG, "unknown id:%" PRId32 "", event_id);
        break;
    }
}
void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
}

void app_main(void)
{
    /* Inicializacao do LwIP e Wi-Fi */

    ESP_ERROR_CHECK(nvs_flash_init()); // inicializar particao NVS
    ESP_ERROR_CHECK(esp_netif_init()); // inicializar pilha de protocolos TCP/IP (LwIP)
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // inicializar modulo de eventos para lidar com eventos IP e mesh
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL)); // criar interface de rede 
    
    wifi_init_config_t cfg_wifi = WIFI_INIT_CONFIG_DEFAULT(); // configuracao padrao para wifi
    ESP_ERROR_CHECK(esp_wifi_init(&cfg_wifi)); // submeter configuracoes wi-fi
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL)); // registrar funcao para lidar com eventos na stack IP
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH)); // definir o tipo de armazenamento utilizado pela API do Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start()); // iniciar wi-fi

    /* Inicializacao da Mesh */

    static const uint8_t MESH_ID[6] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0x77}; // endereço MAC da rede mesh (mesh ID)
    static const char ROUTER_SSID[] = "ssi_da_sua_rede_wifi"; // SSID da rede Wi-Fi do roteador
    static const char ROUTER_PWD[] = "senha_da_sua_rede_wifi"; // senha da rede Wi-Fi do roteador
    static const char MESH_AP_PWD[] = "senha_para_o_AP_deste_dispositivo"; // senha do ponto de acesso (AP) deste dispositivo
    
    ESP_ERROR_CHECK(esp_mesh_init()); // inicializar mesh
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL)); // registrar funcao para lidar com eventos mesh
    
    mesh_cfg_t cfg_mesh = MESH_INIT_CONFIG_DEFAULT(); // configuracao padrao para a mesh
    
    // configuracoes da rede mesh
    memcpy((uint8_t *) &cfg_mesh.mesh_id, MESH_ID, 6); // atribuindo mesh ID para struct de configuracao
    
    // configuracoes do roteador
    cfg_mesh.channel = 0; // canal do roteador
    cfg_mesh.router.ssid_len = strlen(ROUTER_SSID); // tamanho da string do SSID
    memcpy((uint8_t *) &cfg_mesh.router.ssid, ROUTER_SSID, cfg_mesh.router.ssid_len); // atribuindo SSID da rede do roteador para struct de configuracao
    memcpy((uint8_t *) &cfg_mesh.router.password, ROUTER_PWD, strlen(ROUTER_PWD)); // atribuindo senha da rede do roteador para struct de configuracao
    
    // configuracoes do softAP mesh
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK)); // modo de autenticacao do softAP
    cfg_mesh.mesh_ap.max_connection = 2; // numero max. de estacoes conectadas ao softAP
    cfg_mesh.mesh_ap.nonmesh_max_connection = 1; // numero max. de conexoes nao-mesh
    memcpy((uint8_t *) &cfg_mesh.mesh_ap.password, MESH_AP_PWD, strlen(MESH_AP_PWD)); // atribuir senha do softAP

    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg_mesh)); // submeter configuracoes mesh
    ESP_ERROR_CHECK(esp_mesh_start()); // iniciar mesh
    ESP_ERROR_CHECK(esp_mesh_set_self_organized(true, true)); // rede autoconfiguravel (padrao)

    ESP_LOGI(MESH_TAG, "rede mesh inicializada com sucesso");
}
