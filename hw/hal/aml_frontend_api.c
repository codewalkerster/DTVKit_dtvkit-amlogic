#include "aml_frontend_api.h"

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "dtv_log.h"
#define TAG  "FRONTEND_API"

#define FD_API_ERR(x,...)  DTV_LOGE(TAG, "[%s:%d] " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FD_API_INFO(x,...) DTV_LOGI(TAG, "[%s:%d] " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)

static BOOLEAN set_property_data(S32BIT fd, U32BIT cmd, U32BIT data);
static BOOLEAN set_properties (U32BIT fd, struct dtv_properties *props);
static BOOLEAN get_property_data(S32BIT fd, U32BIT cmd, U32BIT *data);
static BOOLEAN get_property_data_ex(S32BIT fd, U32BIT cmd, U32BIT *data, U32BIT* reserved);
static BOOLEAN get_property_data_array(S32BIT fd, U32BIT cmd, U8BIT *data, U32BIT *len);

static BOOLEAN set_property_data(S32BIT fd, U32BIT cmd, U32BIT data)
{
    struct dtv_property prop;
    struct dtv_properties props;

    memset(&prop, 0, sizeof(struct dtv_property));

    prop.cmd = cmd;
    prop.u.data = data;

    props.num = 1;
    props.props = &prop;

    if (ioctl(fd, FE_SET_PROPERTY, &props) < 0)
    {
        FD_API_ERR("Fail to set property (fd:%d cmd:%u data:%u) errno %d (%s)", fd, cmd, data, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to set property (fd:%d cmd:%u data:%u)", fd, cmd, data);

    return TRUE;
}

static BOOLEAN set_properties(U32BIT fd, struct dtv_properties *props)
{
    if (ioctl(fd, FE_SET_PROPERTY, props) < 0)
    {
        FD_API_ERR("Fail to set properties (fd:%d) errno %d (%s)", fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to set properties (fd:%d)", fd);

    return TRUE;
}

static BOOLEAN get_property_data(S32BIT fd, U32BIT cmd, U32BIT *data)
{
    struct dtv_property prop;
    struct dtv_properties props;

    memset(&prop, 0, sizeof(struct dtv_property));

    prop.cmd = cmd;
    props.num = 1;
    props.props = &prop;

    if (ioctl(fd, FE_GET_PROPERTY, &props) < 0)
    {
        FD_API_ERR("Fail to get property (fd:%d cmd:%u) errno %d (%s)", fd, cmd, errno, strerror(errno));
        return FALSE;
    }

    *data = prop.u.data;

    FD_API_INFO("Okay to get property (fd:%d cmd:%u data:%u)", fd, cmd, *data);

    return TRUE;
}

static BOOLEAN get_property_data_ex(S32BIT fd, U32BIT cmd, U32BIT *data, U32BIT* reserved /*array size should be 3*/)
{
    struct dtv_property prop;
    struct dtv_properties props;

    memset(&prop, 0, sizeof(struct dtv_property));

    prop.cmd = cmd;
    props.num = 1;
    props.props = &prop;

    if (ioctl(fd, FE_GET_PROPERTY, &props) < 0)
    {
        FD_API_ERR("Fail to get property (fd:%d cmd:%u) errno %d (%s)", fd, cmd, errno, strerror(errno));
        return FALSE;
    }

    *data = prop.u.data;
    reserved[0] = prop.reserved[0];
    reserved[1] = prop.reserved[1];
    reserved[2] = prop.reserved[2];

    FD_API_INFO("Okay to get property (fd:%d cmd:%u data:%u reserved:%u %u %u)",
                fd, cmd, *data, reserved[0], reserved[1], reserved[2]);

    return TRUE;
}

static BOOLEAN get_property_data_array(S32BIT fd, U32BIT cmd, U8BIT *data, U32BIT *len)
{
    struct dtv_property prop;
    struct dtv_properties props;

    memset(&prop, 0, sizeof(struct dtv_property));

    prop.cmd = cmd;
    props.num = 1;
    props.props = &prop;

    if (ioctl(fd, FE_GET_PROPERTY, &props) < 0)
    {
        FD_API_ERR("Fail to get property array (fd:%d cmd:%u) errno %d (%s)", fd, cmd, errno, strerror(errno));
        return FALSE;
    }

    for (U32BIT i = 0; i < prop.u.buffer.len; i++)
    {
        FD_API_INFO("buffer: %d", prop.u.buffer.data[i]);
    }

    memcpy(data, prop.u.buffer.data, prop.u.buffer.len);
    *len = prop.u.buffer.len;

    FD_API_INFO("Okay to get property array (fd:%d cmd:%u data:%p len:%u)", fd, cmd, data, *len);

    return TRUE;
}

BOOLEAN aml_frontend_set_fe_property(S32BIT frontend_fd, fe_delivery_system_t fe_mode)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!set_property_data(frontend_fd, DTV_DELIVERY_SYSTEM, fe_mode))
    {
        FD_API_ERR("Fail to set frontend property (fd:%d mode:%u)", frontend_fd, fe_mode);
        return FALSE;
    }

    FD_API_INFO("Okay to set frontend property (fd:%d mode:%u)", frontend_fd, fe_mode);

    return TRUE;
}

BOOLEAN aml_frontend_get_fe_property(S32BIT frontend_fd, fe_delivery_system_t *fe_mode_ptr)
{
    U32BIT data = 0;

    if (INVALID_FD == frontend_fd)
    {
        return FALSE;
    }

    if (NULL == fe_mode_ptr)
    {
        return FALSE;
    }

    if (get_property_data(frontend_fd, DTV_DELIVERY_SYSTEM, &data))
    {
        *fe_mode_ptr = (fe_delivery_system_t)data;
    }
    else
    {
        *fe_mode_ptr = SYS_UNDEFINED;

        return FALSE;
    }

    return TRUE;
}


BOOLEAN aml_frontend_get_tsinput(S32BIT frontend_fd, U32BIT *index)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!get_property_data(frontend_fd, DTV_TS_INPUT, index))
    {
        FD_API_ERR("Fail to get tsinput (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to get tsinput (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_get_support_delivery_system_list(S32BIT frontend_fd, U8BIT *system_list, U32BIT *list_len)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!get_property_data_array(frontend_fd, DTV_ENUM_DELSYS, system_list, list_len))
    {
        FD_API_ERR("Fail to get supported delivery system list (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to get supported delivery system list (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_get_signal_strength(S32BIT frontend_fd, U16BIT *strength)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (ioctl(frontend_fd, FE_READ_SIGNAL_STRENGTH, strength) < 0)
    {
        FD_API_ERR("Fail to read signal strength (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to read signal strength (fd:%d strength:%u)", frontend_fd, *strength);

    return TRUE;
}

BOOLEAN aml_frontend_get_signal_strength_property(S32BIT frontend_fd, U16BIT *strength, U16BIT *dBmV)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    struct dtv_property prop;
    struct dtv_properties props;

    memset(&prop, 0, sizeof(struct dtv_property));

    /* start get Signal Strength .*/
    prop.cmd = DTV_STAT_SIGNAL_STRENGTH;
    props.num = 1;
    props.props = &prop;

    if (ioctl(frontend_fd, FE_GET_PROPERTY, &props) < 0)
    {
        FD_API_ERR("Fail to get signal property (fd:%d cmd:%u) errno %d (%s)",
                   frontend_fd, prop.cmd, errno, strerror(errno));
        return FALSE;
    }

    U16BIT strength_value = 0;
    U16BIT dBmV_value = 0;

    U8BIT len = prop.u.st.len;
    for (U8BIT i = 0; i < len; i++)
    {
        U8BIT scale = prop.u.st.stat[i].scale;
        U64BIT value = prop.u.st.stat[i].uvalue;
        if (scale == FE_SCALE_RELATIVE)
        {
            strength_value = (U16BIT)value;
        }
        else if (scale == FE_SCALE_DECIBEL)
        {
            dBmV_value = (U16BIT)value;
        }
    }

    FD_API_INFO("Okay to get signal property (fd:%d strength:%u dBmV:%u)",
                frontend_fd, strength_value, dBmV_value);

    if (strength != NULL)
    {
        *strength = strength_value;
    }

    if (dBmV != NULL)
    {
        *dBmV = dBmV_value;
    }

    return TRUE;
}


BOOLEAN aml_frontend_get_signal_ber(S32BIT frontend_fd, U32BIT *ber)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (ioctl(frontend_fd, FE_READ_BER, ber) < 0)
    {
        FD_API_ERR("Fail to read signal ber (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to read signal ber (fd:%d ber:%u)", frontend_fd, *ber);

    return TRUE;
}

BOOLEAN aml_frontend_get_signal_snr(S32BIT frontend_fd, U16BIT *snr)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (ioctl(frontend_fd, FE_READ_SNR, snr) < 0)
    {
        FD_API_ERR("Fail to read signal snr (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to read signal snr (fd:%d snr:%u)", frontend_fd, *snr);

    return TRUE;
}

BOOLEAN aml_frontend_get_frequency(S32BIT frontend_fd, U32BIT *frequency)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!get_property_data(frontend_fd, DTV_FREQUENCY, frequency))
    {
        FD_API_ERR("Fail to get frequency (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to get frequency (fd:%d frequency:%u)", frontend_fd, *frequency);

    return TRUE;
}

BOOLEAN aml_frontend_get_transmission_mode(S32BIT frontend_fd, U32BIT *mode)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!get_property_data(frontend_fd, DTV_TRANSMISSION_MODE, mode))
    {
        FD_API_ERR("Fail to get transmission mode (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to get transmission mode (fd:%d mode:%u)", frontend_fd, *mode);

    return TRUE;
}

BOOLEAN aml_frontend_get_terr_constellation(S32BIT frontend_fd, U32BIT *fe_mode, U32BIT *constellation)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    U32BIT data;
    U32BIT reserved[3] = {0};
    if (!get_property_data_ex(frontend_fd, DTV_DELIVERY_SYSTEM, &data, reserved))
    {
        FD_API_ERR("Fail to get terr constellation (fd:%d)", frontend_fd);
        return FALSE;
    }

    *fe_mode = data;
    *constellation = reserved[0];

    FD_API_INFO("Okay to get terr constellation (fd:%d mode:%u constellation:%u)",
                frontend_fd, *fe_mode, *constellation);

    return TRUE;
}

BOOLEAN aml_frontend_get_terr_hierarchy(S32BIT frontend_fd, U32BIT *hierarchy)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!get_property_data(frontend_fd, DTV_HIERARCHY, hierarchy))
    {
        FD_API_ERR("Fail to get terr hierarchy (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to get terr hierarchy (fd:%d hierarchy:%u)", frontend_fd, *hierarchy);

    return TRUE;
}

BOOLEAN aml_frontend_get_dvbt2_plp_id_list(S32BIT frontend_fd, U32BIT max_plp, U8BIT *plp_ids, U32BIT *list_len)
{
    struct dtv_property prop;
    memset(&prop, 0, sizeof(struct dtv_property));

    prop.cmd = DTV_DVBT2_PLP_ID;
    prop.u.buffer.reserved1[1] = max_plp;
    prop.u.buffer.reserved2 = plp_ids;

    struct dtv_properties props;
    props.num = 1;
    props.props = &prop;

    if (ioctl(frontend_fd, FE_GET_PROPERTY, &props) < 0)
    {
        FD_API_ERR("Fail to get dvbt2 plp id list (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    *list_len = prop.u.buffer.reserved1[0];

    FD_API_INFO("Okay to get dvbt2 plp id list (fd:%d plp_ids:%p len:%u)", frontend_fd, *plp_ids, *list_len);

    return TRUE;
}

BOOLEAN aml_frontend_set_dvbt2_plp_id(S32BIT frontend_fd, U8BIT plp_id)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!set_property_data(frontend_fd, DTV_DVBT2_PLP_ID, plp_id))
    {
        FD_API_ERR("Fail to set dvbt2 plp id (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to set dvbt2 plp id (fd:%d id:%d)", frontend_fd, plp_id);

    return TRUE;
}

BOOLEAN aml_frontend_get_terr_coderate(S32BIT frontend_fd, U32BIT *fe_mode, U32BIT *coderate)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    U32BIT data;
    U32BIT reserved[3] = {0};
    if (!get_property_data_ex(frontend_fd, DTV_DELIVERY_SYSTEM, &data, reserved))
    {
        FD_API_ERR("Fail to get terr coderate (fd:%d)", frontend_fd);
        return FALSE;
    }

    *fe_mode = data;
    *coderate = reserved[1];

    FD_API_INFO("Okay to get terr coderate (fd:%d mode:%u coderate:%u)", frontend_fd, *fe_mode, *coderate);

    return TRUE;
}

BOOLEAN aml_frontend_get_terr_cellid(S32BIT frontend_fd, U32BIT *fe_mode, U32BIT *cellid)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    U32BIT data;
    U32BIT reserved[3] = {0};
    if (!get_property_data_ex(frontend_fd, DTV_DELIVERY_SYSTEM, &data, reserved))
    {
        FD_API_ERR("Fail to get terr cellid (fd:%d)", frontend_fd);
        return FALSE;
    }

    *fe_mode = data;
    *cellid = reserved[2];

    FD_API_INFO("Okay to get terr cellid (fd:%d mode:%u cellid:%u)", frontend_fd, *fe_mode, *cellid);

    return TRUE;
}

BOOLEAN aml_frontend_get_delivery_system(S32BIT frontend_fd, U32BIT *system, U32BIT *modulation, U32BIT *srate)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    U32BIT data;
    U32BIT reserved[3] = {0};
    if (!get_property_data_ex(frontend_fd, DTV_DELIVERY_SYSTEM, &data, reserved))
    {
        FD_API_ERR("Fail to get delivery system (fd:%d)", frontend_fd);
        return FALSE;
    }

    *system = data;
    *modulation = reserved[0];  // reserved[0] is used for modulation in demod
    *srate = reserved[1];  // reserved[1] is used for symbol rate in demod

    FD_API_INFO("Okay to get delivery system (fd:%d system:%u modulation:%u srate:%u)",
                frontend_fd, *system, *modulation, *srate);

    return TRUE;
}

BOOLEAN aml_frontend_set_voltage(S32BIT frontend_fd, fe_sec_voltage_t voltage)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (ioctl(frontend_fd, FE_SET_VOLTAGE, voltage) < 0)
    {
        FD_API_ERR("Fail to set voltage (fd:%d voltage:%u) errno %d (%s)", frontend_fd, voltage, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to set voltage (fd:%d voltage:%u)", frontend_fd, voltage);

    return TRUE;
}

BOOLEAN aml_frontend_set_tone(S32BIT frontend_fd, BOOLEAN use_22khz)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    fe_sec_tone_mode_t tone = SEC_TONE_OFF;
    if (use_22khz)
    {
        tone = SEC_TONE_ON;
    }
    else
    {
        tone = SEC_TONE_OFF;
    }

    if (ioctl(frontend_fd, FE_SET_TONE, tone) < 0)
    {
        FD_API_ERR("Fail to set tone (fd:%d 22khz:%u tone:%u) errno %d (%s)", frontend_fd, use_22khz, tone, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to set tone (fd:%d 22khz:%u tone:%u)", frontend_fd, use_22khz, tone);

    return TRUE;
}

BOOLEAN aml_frontend_master_send_diseqc_cmd(S32BIT frontend_fd, U8BIT *data, U8BIT data_len)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    struct dvb_diseqc_master_cmd cmd;
    memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

    for (U8BIT i = 0; i < data_len; i++)
    {
        cmd.msg[i] = data[i];
        FD_API_INFO("cmd:0x%02x", data[i]);
    }

    cmd.msg_len = data_len;

    if (ioctl(frontend_fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) < 0)
    {
        FD_API_ERR("Fail to send diseqc cmd (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to send diseqc cmd (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_slave_receive_diseqc_reply(S32BIT frontend_fd, U8BIT *data, U8BIT data_len, U32BIT timeout)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    struct dvb_diseqc_slave_reply reply;
    memset(&reply, 0, sizeof(struct dvb_diseqc_slave_reply));

    reply.timeout = (int)timeout;
    if (ioctl(frontend_fd, FE_DISEQC_RECV_SLAVE_REPLY, &reply) < 0)
    {
        FD_API_ERR("Fail to receive diseqc reply (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    for (U8BIT i = 0; i < reply.msg_len; i++)
    {
         FD_API_INFO("reply:0x%02x", reply.msg[i]);
    }

    if (data_len >= reply.msg_len)
    {
        memcpy(data, reply.msg, reply.msg_len);
    }
    else
    {
        FD_API_ERR("Diseqc reply buffer is not enough (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to receive diseqc reply (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_send_diseqc_burst(S32BIT frontend_fd, U8BIT data)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (data != 0x00 && data != 0xFF)
    {
        FD_API_ERR("diseqc burst data is incorrect (fd:%d data:%u)", frontend_fd, data);
        return FALSE;

    }

    fe_sec_mini_cmd_t cmd = SEC_MINI_A;
    if (data == 0x00)
    {
        cmd = SEC_MINI_A;
    }
    else
    {
        cmd = SEC_MINI_B;
    }

    if (ioctl(frontend_fd, FE_DISEQC_SEND_BURST, cmd) < 0)
    {
        FD_API_ERR("Fail to send diseqc cmd (fd:%d data:0x%x cmd:%u) errno %d (%s)",
                   frontend_fd, data, cmd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay send diseqc burst (fd:%d data:0x%x cmd:%u)", frontend_fd, data, cmd);

    return TRUE;
}

BOOLEAN aml_frontend_blindscan_start(S32BIT frontend_fd, struct dvbsx_blindscanpara *pbspara)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    struct dtv_property property[8];
    memset(property, 0, 8 * sizeof(struct dtv_property));

    /*set min fre*/
    property[0].cmd = DTV_BLIND_SCAN_MIN_FRE;
    property[0].u.data = pbspara->minfrequency;

    /*set max fre*/
    property[1].cmd = DTV_BLIND_SCAN_MAX_FRE;
    property[1].u.data = pbspara->maxfrequency;

    /*set min rate*/
    property[2].cmd = DTV_BLIND_SCAN_MIN_SRATE;
    property[2].u.data = pbspara->minSymbolRate;

    /*set max rate*/
    property[3].cmd = DTV_BLIND_SCAN_MAX_SRATE;
    property[3].u.data = pbspara->maxSymbolRate;
    /*set fre range*/
    property[4].cmd = DTV_BLIND_SCAN_FRE_RANGE;
    property[4].u.data = pbspara->frequencyRange;
    /*set fre step*/
    property[5].cmd = DTV_BLIND_SCAN_FRE_STEP;
    property[5].u.data = pbspara->frequencyStep;
    /*set time out*/
    property[6].cmd = DTV_BLIND_SCAN_TIMEOUT;
    property[6].u.data = pbspara->timeout;
    /*set start blind scan*/
    property[7].cmd = DTV_START_BLIND_SCAN;
    property[7].u.data = 0;

    for (U8BIT i = 0; i < 8; i++)
    {
        FD_API_INFO("set start blind num[%d] cmd[%d] data[%d]", i, property[i].cmd, property[i].u.data);
    }

    struct dtv_properties props;
    props.num = 8;
    props.props = property;

    if (!set_properties(frontend_fd, &props))
    {
        FD_API_ERR("Fail to start blind scan (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to start blind scan (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_blindscan_next(S32BIT frontend_fd)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!set_property_data(frontend_fd, DTV_BLIND_SCAN_STEP_NEXT, 0))
    {
        FD_API_ERR("Fail to next blind scan (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to next blind scan (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_blindscan_cancel(S32BIT frontend_fd)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!set_property_data(frontend_fd, DTV_CANCEL_BLIND_SCAN, 0))
    {
        FD_API_ERR("Fail to cancel blind scan (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to cancel blind scan (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_blindscan_set_singlecable(S32BIT frontend_fd, struct dvbsx_singlecable_parameters *singlecablePara)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    struct dtv_property property[6];
    memset(property, 0, 6 * sizeof(struct dtv_property));

    /*set singlecable*/
    property[0].cmd = DTV_SINGLE_CABLE_VER;
    property[0].u.data = singlecablePara->version;
    property[1].cmd = DTV_SINGLE_CABLE_USER_BAND;
    property[1].u.data = singlecablePara->userband;
    property[2].cmd = DTV_SINGLE_CABLE_BAND_FRE;
    property[2].u.data = singlecablePara->frequency;
    property[3].cmd = DTV_SINGLE_CABLE_BANK;
    property[3].u.data = singlecablePara->bank;
    property[4].cmd = DTV_SINGLE_CABLE_UNCOMMITTED;
    property[4].u.data = singlecablePara->uncommitted;
    property[5].cmd = DTV_SINGLE_CABLE_COMMITTED;
    property[5].u.data = singlecablePara->committed;

    for (U8BIT i = 0; i < 6; i++)
    {
        FD_API_INFO("set start singlecable num[%d] cmd[%d] data[%d]", i, property[i].cmd, property[i].u.data);
    }

    struct dtv_properties props;
    props.num = 6;
    props.props = property;

    if (!set_properties(frontend_fd, &props))
    {
        FD_API_ERR("Fail to set singlecable (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to set singlecable (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_get_frontend_info(S32BIT frontend_fd, struct dvb_frontend_info *fe_info)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (ioctl(frontend_fd, FE_GET_INFO, fe_info) < 0)
    {
        FD_API_ERR("Fail to get frontend info (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to get frontend info (fd:%d)", frontend_fd);

    return TRUE;
}

S32BIT aml_frontend_open_tuner(U8BIT fe_index)
{
    char fe_name[32];
    struct stat file_status;
    S32BIT fd = INVALID_FD;

    memset(fe_name, 0, sizeof(fe_name));
    snprintf(fe_name, sizeof(fe_name), "/dev/dvb0.frontend%u", fe_index);

    if (stat(fe_name, &file_status) == 0)
    {
        if ((fd = open(fe_name, O_RDWR | O_NONBLOCK)) < 0)
        {
            FD_API_ERR("Fail to open tune[%u] %s, errno %d (%s)", fe_index, fe_name, errno, strerror(errno));
            fd = INVALID_FD;
        }
        else
        {
            FD_API_INFO("Okay to open tuner[%u] %s", fe_index, fe_name);
        }
    }
    else
    {
        FD_API_ERR("Not found tuner[%u] %s", fe_index, fe_name);
    }

    return fd;
}

BOOLEAN aml_frontend_close_tuner(S32BIT frontend_fd)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (close(frontend_fd) < 0)
    {
        FD_API_ERR("Fail to close tuner (fd:%d), errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to close tuner (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_clear_tuner(S32BIT frontend_fd)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (!set_property_data(frontend_fd, DTV_CLEAR, 0))
    {
        FD_API_ERR("Fail to clear tuner (fd:%d)", frontend_fd);
        return FALSE;
    }

    FD_API_INFO("Okay to clear tuner (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_set_frontend(S32BIT frontend_fd, struct dvb_frontend_parameters *fe_params)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (ioctl(frontend_fd, FE_SET_FRONTEND, fe_params) < 0)
    {
        FD_API_ERR("Fail to set frontend (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to set frontend (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_get_event(S32BIT frontend_fd, struct dvb_frontend_event *fe_event)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (ioctl(frontend_fd, FE_GET_EVENT, fe_event) < 0)
    {
        FD_API_ERR("Fail to get event (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to get event (fd:%d)", frontend_fd);

    return TRUE;
}

BOOLEAN aml_frontend_get_tuner_status(S32BIT frontend_fd, U32BIT *fe_status)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    if (ioctl(frontend_fd, FE_READ_STATUS, fe_status) < 0)
    {
        FD_API_ERR("Fail to get status (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    FD_API_INFO("Okay to get status (fd:%d status:%u)", frontend_fd, *fe_status);

    return TRUE;
}

BOOLEAN aml_frontend_get_isdbt_partial_reception(S32BIT frontend_fd, U32BIT *sys_id, U32BIT *ewbs_flag)
{
    if (frontend_fd == INVALID_FD)
        return FALSE;

    struct dtv_property prop;
    memset(&prop, 0, sizeof(struct dtv_property));
    prop.cmd = DTV_ISDBT_PARTIAL_RECEPTION;

    struct dtv_properties props;
    props.num = 1;
    props.props = &prop;
    if (ioctl(frontend_fd, FE_GET_PROPERTY, &props) < 0)
    {
        FD_API_ERR("Fail to get isdbt partial reception (fd:%d) errno %d (%s)", frontend_fd, errno, strerror(errno));
        return FALSE;
    }

    *sys_id = prop.u.buffer.reserved1[0];
    *ewbs_flag = prop.u.buffer.reserved1[1];

    FD_API_INFO("Okay to get isdbt partial reception (fd:%d data:%u %u)", frontend_fd, *sys_id, *ewbs_flag);

    return TRUE;
}
