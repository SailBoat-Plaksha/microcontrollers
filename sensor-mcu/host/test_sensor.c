#include "sensor_node.h"
#include "mailbox_transport.h"
#include "pico/i2c_slave.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now_ms;
static uint32_t clock_fn(void*x){(void)x;return now_ms;}
static size_t frame(mavlink_message_t*m,uint8_t out[64]){uint8_t t[MAVLINK_MAX_PACKET_LEN];uint16_t n=mavlink_msg_to_send_buffer(t,m);assert(n<=64);memset(out,0,64);memcpy(out,t,n);return n;}
static void parse(uint8_t in[64],mavlink_message_t*out){mavlink_status_t s={0};int found=0;for(int i=0;i<64;i++)if(mavlink_parse_char(MAVLINK_COMM_1,in[i],out,&s))found++;assert(found==1);}
static void send_req(sensor_node_t*n,uint32_t id,uint16_t maxage){mavlink_message_t m;mavlink_msg_imu_request_pack(1,1,&m,42,id,80,maxage,1,1,159);uint8_t b[64];frame(&m,b);assert(sensor_node_receive(n,b));}
int main(void){sensor_node_t n;sensor_config_t c={clock_fn,NULL,0x1234,10,0,1};sensor_node_init(&n,&c);uint8_t b[64];mavlink_message_t m;
    send_req(&n,9,0);assert(sensor_node_take_tx(&n,b));parse(b,&m);assert(m.msgid==MAVLINK_MSG_ID_IMU_REQUEST&&m.sysid==1&&m.compid==159);mavlink_imu_request_t r;mavlink_msg_imu_request_decode(&m,&r);assert(r.path==0&&r.target_system==1&&r.target_component==1&&r.ground_session==42&&r.request_id==9);
    mavlink_msg_imu_sample_pack(1,1,&m,42,9,12345,8,777,777,5,5,100,200,300,10,20,30,40,50,60,1,0,1,1,159);frame(&m,b);now_ms=20;assert(sensor_node_receive(&n,b));assert(sensor_node_take_tx(&n,b));parse(b,&m);assert(m.msgid==MAVLINK_MSG_ID_IMU_SAMPLE&&m.sysid==1&&m.compid==159);mavlink_imu_sample_t s;mavlink_msg_imu_sample_decode(&m,&s);assert(s.path==1&&s.acquisition_component==1&&s.source_boot_id==0x1234&&s.acquisition_boot_id==777&&s.time_boot_ms==12345&&s.sample_seq==8&&s.sample_age_ms==5&&s.valid_fields==5);assert(s.xgyro==0&&s.ygyro==0&&s.zgyro==0&&s.xacc==100&&s.zmag==60);
    send_req(&n,9,0);assert(sensor_node_take_tx(&n,b));parse(b,&m);assert(m.msgid==MAVLINK_MSG_ID_IMU_SAMPLE);assert(n.count==0);
    send_req(&n,9,2);assert(!sensor_node_take_tx(&n,b)); /* altered duplicate is rejected without another dependency */ assert(n.rejected==1);
    send_req(&n,14,7);assert(sensor_node_take_tx(&n,b));parse(b,&m);assert(m.msgid==MAVLINK_MSG_ID_IMU_SAMPLE);mavlink_msg_imu_sample_decode(&m,&s);assert(s.sample_age_ms==5&&s.sample_seq==8);
    now_ms=30;send_req(&n,15,8);assert(sensor_node_take_tx(&n,b));parse(b,&m);assert(m.msgid==MAVLINK_MSG_ID_IMU_REQUEST); /* cached source age plus Pico residence exceeded max_age */
    send_req(&n,10,0);assert(sensor_node_take_tx(&n,b));now_ms=110; sensor_node_tick(&n);assert(sensor_node_take_tx(&n,b));parse(b,&m);assert(m.msgid==MAVLINK_MSG_ID_IMU_SAMPLE);mavlink_msg_imu_sample_decode(&m,&s);assert(s.status==4&&s.valid_fields==0);assert(sensor_node_take_tx(&n,b));parse(b,&m);assert(m.msgid==MAVLINK_MSG_ID_IMU_SAMPLE);assert(n.timeouts==2);
    /* Invalid source, target, path, boot ownership, status/mask, and bad CRC are rejected. */
    uint32_t rejected=n.rejected;mavlink_msg_imu_request_pack(2,1,&m,42,12,50,0,1,1,159);frame(&m,b);sensor_node_receive(&n,b);assert(n.rejected==rejected+1);
    mavlink_msg_imu_request_pack(1,1,&m,42,12,50,0,1,1,158);frame(&m,b);sensor_node_receive(&n,b);assert(n.rejected==rejected+2);
    mavlink_msg_imu_request_pack(1,1,&m,42,12,50,0,0,1,159);frame(&m,b);sensor_node_receive(&n,b);assert(n.rejected==rejected+3);
    send_req(&n,12,0);assert(sensor_node_take_tx(&n,b));
    mavlink_msg_imu_sample_pack(1,1,&m,42,12,150,10,123,456,0,1,1,2,3,0,0,0,0,0,0,1,0,1,1,159);frame(&m,b);sensor_node_receive(&n,b);assert(n.rejected==rejected+4);
    mavlink_msg_imu_sample_pack(1,1,&m,42,12,150,10,123,123,0,1,1,2,3,0,0,0,0,0,0,2,0,1,1,159);frame(&m,b);sensor_node_receive(&n,b);assert(n.rejected==rejected+5);
    mavlink_msg_ping_pack(255,190,&m,999,3,1,159);frame(&m,b);assert(sensor_node_receive(&n,b));assert(sensor_node_take_tx(&n,b));parse(b,&m);assert(m.msgid==MAVLINK_MSG_ID_PING&&m.sysid==1&&m.compid==159);mavlink_ping_t ping;mavlink_msg_ping_decode(&m,&ping);assert(ping.target_system==255&&ping.target_component==190&&ping.seq==3);
    mavlink_msg_ping_pack(255,190,&m,1000,4,9,159);frame(&m,b);sensor_node_receive(&n,b);assert(!sensor_node_take_tx(&n,b));assert(n.rejected==rejected+6);
    mavlink_msg_imu_request_pack(1,1,&m,42,13,50,0,1,1,159);size_t wire=frame(&m,b);b[wire-1]^=0x80;uint32_t malformed=n.malformed;assert(!sensor_node_receive(&n,b));assert(n.malformed==malformed+1);
    /* Session-zero local API produces a Pi-bound dependency and Pi-bound result. */
    assert(sensor_node_request_local(&n,90,50,0));assert(sensor_node_take_tx(&n,b));parse(b,&m);mavlink_msg_imu_request_decode(&m,&r);assert(m.sysid==1&&m.compid==159&&r.ground_session==0&&r.path==0&&r.target_system==1&&r.target_component==1);
    mavlink_msg_imu_sample_pack(1,1,&m,0,90,500,12,987,987,0,1,9,8,7,0,0,0,0,0,0,1,0,1,1,159);frame(&m,b);now_ms=120;assert(sensor_node_receive(&n,b));assert(sensor_node_take_tx(&n,b));parse(b,&m);mavlink_msg_imu_sample_decode(&m,&s);assert(s.ground_session==0&&s.path==1&&s.target_system==1&&s.target_component==1&&s.source_boot_id==0x1234);
    /* An older Pi acquisition from the same boot cannot roll cached freshness back. */
    assert(sensor_node_request_local(&n,91,50,0));assert(sensor_node_take_tx(&n,b));
    mavlink_msg_imu_sample_pack(1,1,&m,0,91,499,11,987,987,0,1,1,2,3,4,5,6,7,8,9,1,0,1,1,159);frame(&m,b);
    uint32_t stale_rejected=n.rejected;assert(sensor_node_receive(&n,b));assert(n.rejected==stale_rejected+1);assert(!sensor_node_take_tx(&n,b));
    /* Once the 2 s cache expires, even a permissive max-age request asks Pi for fresh data. */
    now_ms=2121;assert(sensor_node_request_local(&n,92,50,65535));assert(sensor_node_take_tx(&n,b));parse(b,&m);
    assert(m.msgid==MAVLINK_MSG_ID_IMU_REQUEST);mavlink_msg_imu_request_decode(&m,&r);assert(r.ground_session==0&&r.request_id==92&&r.max_age_ms==0);
    /* The actual callback latches a reply for each transaction. A short read
     * and a write-only STOP leave it prepared; a full read consumes it. */
    sensor_mailbox_t mb;sensor_mailbox_init(&mb);memset((void*)mb.tx,0xA5,64);mb.tx_available=true;i2c_inst_t bus={0};
    for(int i=0;i<7;i++)sensor_mailbox_callback(&bus,I2C_SLAVE_REQUEST);
    sensor_mailbox_callback(&bus,I2C_SLAVE_FINISH);assert(mb.tx_available);
    mavlink_msg_imu_request_pack(1,1,&m,42,11,30,0,1,1,159);frame(&m,b);bus.input=b;bus.input_len=64;bus.input_pos=0;bus.output_len=0;
    for(int i=0;i<64;i++)sensor_mailbox_callback(&bus,I2C_SLAVE_RECEIVE);
    sensor_mailbox_callback(&bus,I2C_SLAVE_FINISH);assert(mb.rx_ready&&mb.tx_available);
    bus.input=NULL;bus.input_len=bus.input_pos=bus.output_len=0;
    for(int i=0;i<64;i++)sensor_mailbox_callback(&bus,I2C_SLAVE_REQUEST);
    sensor_mailbox_callback(&bus,I2C_SLAVE_REQUEST);assert(bus.output[0]==0xA5&&bus.output[64]==0);
    sensor_mailbox_callback(&bus,I2C_SLAVE_FINISH);assert(!mb.tx_available);
    /* An unready read stays zero even if a reply is prepared mid-transaction. */
    mb.tx_available=false;bus.output_len=0;sensor_mailbox_callback(&bus,I2C_SLAVE_REQUEST);memset((void*)mb.tx,0x5A,64);mb.tx_available=true;sensor_mailbox_callback(&bus,I2C_SLAVE_REQUEST);assert(bus.output[0]==0&&bus.output[1]==0);sensor_mailbox_callback(&bus,I2C_SLAVE_FINISH);assert(mb.tx_available);
    /* Repeated-start write then read finalizes both phases at the single STOP. */
    memset((void*)mb.tx,0x3C,64);mb.tx_available=true;bus.input=b;bus.input_len=64;bus.input_pos=0;bus.output_len=0;
    for(int i=0;i<64;i++)sensor_mailbox_callback(&bus,I2C_SLAVE_RECEIVE);
    for(int i=0;i<64;i++)sensor_mailbox_callback(&bus,I2C_SLAVE_REQUEST);
    sensor_mailbox_callback(&bus,I2C_SLAVE_FINISH);assert(mb.rx_ready&&!mb.tx_available&&bus.output_len==64&&bus.output[0]==0x3C);
    sensor_mailbox_service(&mb,&n);assert(!mb.rx_ready);
    puts("sensor host tests: PASS");return 0;}
