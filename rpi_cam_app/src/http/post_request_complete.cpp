#include "http/HttpInitializer.hpp"
#include "spdlog/spdlog.h"

#include <filesystem>


/// @brief 요청 마지막에 multipart/form-data를 처리하기 위한 것으로, response message를 수정 할 수 없음 
void http::post_request_complete(void *cls, struct MHD_Connection* conn, void** con_cls, enum MHD_RequestTerminationCode toe)
{
    struct http::connection_info* con_info = static_cast<struct http::connection_info*>(*con_cls);
    http::EventHandlerHTTP* _handler = http::EventHandlerHTTP::get_instance();

    struct MHD_Response* response = nullptr;
    std::string response_buffer;

    if(con_info == nullptr) {
        return;
    }

    if(con_info->connection_type == http::HTTP_METHOD::POST) {
        // post processor 삭제
        if(con_info->postprocessor != nullptr) {
            MHD_destroy_post_processor(con_info->postprocessor);
            _handler->upload_client_mutex().lock();
            _handler->upload_client()++;
            _handler->upload_client_mutex().unlock();
        }

        // 빈 값 예외처리
        if(con_info->file_name == "") {
            response_buffer.assign("filename is empty");
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            spdlog::error("filename empty");
            return;
        }
        if(con_info->file_content.empty()) {
            response_buffer.assign("file is empty");
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            spdlog::error("file empty");
            return;
        }
        if(con_info->fps == ""){
            response_buffer.assign("fps is empty");
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            
            spdlog::error("fps is empty");
            return;
        }
        std::string event_directory;
        bool flag = false;

        // 이벤트 디렉터리 생성
        try{
            event_directory.assign("event/");
            event_directory.append(con_info->file_name);
            event_directory.append("/");

            if(std::filesystem::is_directory(event_directory)){
                std::filesystem::remove_all(event_directory);
            }
            std::filesystem::create_directory(event_directory);
        } catch (std::filesystem::filesystem_error& ex){
            flag = false;

            spdlog::error("Invalid file name : {}", ex.what());

            response_buffer.assign("Invalid file name");
            
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            return;
        }

        // 파일 저장
        std::ofstream file(event_directory+con_info->file_name, std::ios::binary);
        if(file.is_open()) {
            file.write(con_info->file_content.data(), con_info->file_content.size());
            file.close();
            spdlog::info("file {} is saved {} bytes", con_info->file_name, con_info->file_content.size());

        }
        else {
            flag=true;
            spdlog::error("File Could not open and save {}", con_info->file_name);
        }
        
        // fps 지정
        std::ofstream fps_file(event_directory+con_info->fps, std::ios::app);
        if(fps_file.is_open() && !flag) {
            spdlog::info("{} fps {}", con_info->file_name, con_info->fps);
            spdlog::info("Create Fps file");
            fps_file.close();
        }
        else {
            flag=true;
            spdlog::error("Cannot Create Fps file");
            fps_file.close();
        }

        // 활성화 여부
        std::ofstream enable_file(event_directory+"enable", std::ios::app);
        if(enable_file.is_open() && !flag) {
            spdlog::info("Enable file");
            enable_file << 1;
            enable_file.close();
        }
        else{
            flag=true;
            spdlog::error("Could not create Info file");
            enable_file.close();
        }

        if(!flag){
            spdlog::info("Program saved Successfully");
            response_buffer.assign("Program saved successfully");

            response = MHD_create_response_from_buffer(
            response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
        }
        else{
            std::filesystem::remove_all(event_directory);
            response_buffer.assign("Program save Error");

            response = MHD_create_response_from_buffer(
            response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
        }
    }
    delete con_info;

    con_cls = nullptr;
}