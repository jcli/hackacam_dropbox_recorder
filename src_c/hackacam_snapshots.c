#include <dirent.h> 
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sdvr_sdk.h"
#include "string.h"

FILE * output_file;

int current_dir_jpg_counter()
{
    DIR           *d;
    struct dirent *dir;
    int jpg_num = 0;
    d = opendir(".");
    if (d)
        {
            while ((dir = readdir(d)) != NULL)
                {
                    //                    printf("%s\n", dir->d_name);
                    int len = strlen(dir->d_name);
                    if (dir->d_name[len-1]=='g' &&
                        dir->d_name[len-2]=='p' &&
                        dir->d_name[len-3]=='j'){
                        jpg_num++;
                    }
                }
            
            closedir(d);
        }
    return jpg_num;
}

void current_date(char* dateTime, unsigned int length)
{
    time_t rawtime;
    struct tm * timeinfo;
    const char *extension = ".jpg";
    memset(dateTime, 0, length);

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    //    printf ( "Current local time and date: %s", asctime (timeinfo) );
    if (length<strlen(asctime(timeinfo))+strlen(extension)+1){
        printf("buffer overflow!\n");
        exit(0);
    }
    memcpy(dateTime, asctime(timeinfo), strlen(asctime(timeinfo))-1);
    for (unsigned int i=0; i<strlen(dateTime); i++){
        if (dateTime[i]==' ' || dateTime[i]==':'){
            dateTime[i]='_';
        }
    }
    memcpy(&dateTime[strlen(dateTime)], extension, strlen(extension)+1);
}

void stream_callback(sdvr_chan_handle_t handle,
                       sdvr_frame_type_e frame_type, sx_uint32 stream_id)
{
    sdvr_av_buffer_t *av_frame;
    sdvr_get_stream_buffer(handle, frame_type, stream_id, &av_frame);

    sx_uint8 *frame_payload;
    sx_uint32 frame_payload_size;

    sdvr_av_buf_payload(av_frame, &frame_payload, &frame_payload_size);
    if (frame_type == SDVR_FRAME_JPEG_SNAPSHOT){
        if (current_dir_jpg_counter()<4){
            char dateTimeName[32];
            current_date(dateTimeName, 32);
            FILE *snapshot_file = fopen(dateTimeName, "wb");
            fwrite(frame_payload, 1, frame_payload_size, snapshot_file);
            fclose(snapshot_file);
        }
        //        printf("number of jpeg is %d\n", current_dir_jpg_counter());

        //        printf("%s\n", dateTime);
    }else{
        //        fwrite(frame_payload, 1, frame_payload_size, output_file);        
    }
    sdvr_release_av_buffer(av_frame);
}

int main()
{
    printf("hackacam hello world.\n");

    sdvr_err_e status;

    status = sdvr_sdk_init();
    if (status){
        printf("sdvr_sdk_init() failed: %s\n", sdvr_get_error_text(status));
    }

    output_file = fopen("./output.h264", "wb");
    sdvr_set_stream_callback(stream_callback);

    char *path = (char *)("resource/s7120ipcam_module_dvrfw.rom");
    status = sdvr_upgrade_firmware(0, path);    
    if (status){
        printf("sdvr_upgrade_firmware() failed: %s\n", sdvr_get_error_text(status));
    }

    printf ("firmware uploaded.\n");

    sdvr_board_settings_t board_settings;
    memset(&board_settings, 0, sizeof(sdvr_board_settings_t));   
    board_settings.hd_video_std = SDVR_VIDEO_STD_1080P30;
    
    status = sdvr_board_connect_ex(0, &board_settings);
    if (status){
        printf("sdvr_board_connect_ex() failed: %s\n", sdvr_get_error_text(status));
    }

    printf ("board connected.\n");

    sdvr_chan_def_t chanDef;
    memset(&chanDef, 0 , sizeof(sdvr_chan_def_t));
    chanDef.video_format_primary   = SDVR_VIDEO_ENC_H264;
    chanDef.board_index            = 0;
    chanDef.chan_num               = 0;
    chanDef.chan_type              = SDVR_CHAN_TYPE_ENCODER;    

    sdvr_chan_buf_def_t chan_buf_counts;
    memset(&chan_buf_counts, 0, sizeof(chan_buf_counts));
    chan_buf_counts.cmd_response_buf_count = 1;
    chan_buf_counts.u1.encoder.video_buf_count = 10;
 
    sdvr_chan_handle_t camera_chan_handle;
    status = sdvr_create_chan_ex (&chanDef, &chan_buf_counts, &camera_chan_handle);
    if (status){
        printf("sdvr_set_video_encoder_channel_params() failed: %s\n", sdvr_get_error_text(status));
    }

    sdvr_video_enc_chan_params_t encParam;
    memset(&encParam, 0, sizeof(sdvr_video_enc_chan_params_t));
    // get default video encoding parameters

    status=sdvr_get_video_encoder_channel_params (camera_chan_handle,
                                                  SDVR_ENC_PRIMARY,
                                                  &encParam);
    if (status){
        printf("sdvr_get_video_encoder_channel_params() failed: %s\n", sdvr_get_error_text(status));
    }

    encParam.res_decimation = SDVR_VIDEO_RES_DECIMATION_EQUAL;
    encParam.encoder.h264.avg_bitrate=6000;
    encParam.encoder.h264.max_bitrate=6000;
    encParam.encoder.h264.bitrate_control=SDVR_BITRATE_CONTROL_CBR_S;
    // set the video encoder parameters.                                                                                                                                                                                                    
    status=sdvr_set_video_encoder_channel_params (camera_chan_handle,
                                                  SDVR_ENC_PRIMARY,
                                                  &encParam);

    if (status){
        printf("sdvr_set_video_encoder_channel_params() failed: %s\n", sdvr_get_error_text(status));
    }
    
    status = sdvr_enable_encoder(camera_chan_handle, SDVR_ENC_PRIMARY, 1);
    if (status){
        printf("sdvr_enable_encoder() failed: %s\n", sdvr_get_error_text(status));
    }

    // test snapshot
    while(1){
        sdvr_snapshot(camera_chan_handle, SDVR_VIDEO_RES_DECIMATION_EQUAL);
        usleep(5000000);
    }

    status = sdvr_enable_encoder(camera_chan_handle, SDVR_ENC_PRIMARY, 0);
    if (status){
        printf("sdvr_enable_encoder() failed: %s\n", sdvr_get_error_text(status));
    }

    status = sdvr_destroy_chan(camera_chan_handle);
    if (status){
        printf("sdvr_destroy_chan() failed: %s\n", sdvr_get_error_text(status));
    }

    status = sdvr_board_disconnect(0);
    if (status){
        printf("sdvr_board_discoonect() failed: %s\n", sdvr_get_error_text(status));
    }

    fclose(output_file);

    return 0;
}
