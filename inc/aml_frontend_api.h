#ifndef __AML_FRONTEND_API_H
#define __AML_FRONTEND_API_H

#include "frontend.h"
#include "techtype.h"

#define INVALID_FD          -1

BOOLEAN aml_frontend_set_fe_property(S32BIT frontend_fd, fe_delivery_system_t fe_mode);
BOOLEAN aml_frontend_get_fe_property(S32BIT frontend_fd, fe_delivery_system_t *fe_mode_ptr);
BOOLEAN aml_frontend_get_tsinput(S32BIT frontend_fd, U32BIT *index);
BOOLEAN aml_frontend_get_support_delivery_system_list(S32BIT frontend_fd, U8BIT *system_list, U32BIT *list_len);
BOOLEAN aml_frontend_get_signal_strength(S32BIT frontend_fd, U16BIT *strength);
BOOLEAN aml_frontend_get_signal_strength_property(S32BIT frontend_fd, U16BIT *strength, U32BIT *dBmV);
BOOLEAN aml_frontend_get_signal_ber(S32BIT frontend_fd, U32BIT *ber);
BOOLEAN aml_frontend_get_signal_snr(S32BIT frontend_fd, U16BIT *snr);
BOOLEAN aml_frontend_get_frequency(S32BIT frontend_fd, U32BIT *frequency);
BOOLEAN aml_frontend_get_transmission_mode(S32BIT frontend_fd, U32BIT *mode);
BOOLEAN aml_frontend_get_terr_constellation(S32BIT frontend_fd, U32BIT *fe_mode, U32BIT *constellation);
BOOLEAN aml_frontend_get_terr_hierarchy(S32BIT frontend_fd, U32BIT *hierarchy);
BOOLEAN aml_frontend_get_dvbt2_plp_id_list(S32BIT frontend_fd, U32BIT max_plp, U8BIT *plp_ids, U32BIT *list_len);
BOOLEAN aml_frontend_set_dvbt2_plp_id(S32BIT frontend_fd, U8BIT plp_id);
BOOLEAN aml_frontend_get_terr_coderate(S32BIT frontend_fd, U32BIT *fe_mode, U32BIT *coderate);
BOOLEAN aml_frontend_get_terr_cellid(S32BIT frontend_fd, U32BIT *fe_mode, U32BIT *cellid);
BOOLEAN aml_frontend_get_delivery_system(S32BIT frontend_fd, U32BIT *system, U32BIT *modulation, U32BIT *srate);
BOOLEAN aml_frontend_set_voltage(S32BIT frontend_fd, fe_sec_voltage_t voltage);
BOOLEAN aml_frontend_set_tone(S32BIT frontend_fd, BOOLEAN use_22khz);
BOOLEAN aml_frontend_master_send_diseqc_cmd(S32BIT frontend_fd, U8BIT *data, U8BIT data_len);
BOOLEAN aml_frontend_slave_receive_diseqc_reply(S32BIT frontend_fd, U8BIT *data, U8BIT data_len, U32BIT timeout);
BOOLEAN aml_frontend_send_diseqc_burst(S32BIT frontend_fd, U8BIT data);
BOOLEAN aml_frontend_blindscan_start(S32BIT frontend_fd, struct dvbsx_blindscanpara *pbspara);
BOOLEAN aml_frontend_blindscan_next(S32BIT frontend_fd);
BOOLEAN aml_frontend_blindscan_cancel(S32BIT frontend_fd);
BOOLEAN aml_frontend_blindscan_set_singlecable(S32BIT frontend_fd, struct dvbsx_singlecable_parameters *singlecablePara);
BOOLEAN aml_frontend_get_frontend_info(S32BIT frontend_fd, struct dvb_frontend_info *fe_info);
S32BIT  aml_frontend_open_tuner(U8BIT fe_index);
BOOLEAN aml_frontend_close_tuner(S32BIT frontend_fd);
BOOLEAN aml_frontend_clear_tuner(S32BIT frontend_fd);
BOOLEAN aml_frontend_set_frontend(S32BIT frontend_fd, struct dvb_frontend_parameters *fe_params);
BOOLEAN aml_frontend_get_event(S32BIT frontend_fd, struct dvb_frontend_event *fe_event);
BOOLEAN aml_frontend_get_tuner_status(S32BIT frontend_fd, U32BIT *fe_event);
BOOLEAN aml_frontend_get_isdbt_partial_reception(S32BIT frontend_fd, U32BIT *sys_id, U32BIT *ewbs_flag);

#endif
