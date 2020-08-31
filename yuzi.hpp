#ifndef _YUZI_HPP_
#define _YUZI_HPP_
#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <memory>
#include <json/json.h>
#include <sstream>
#include "speech/base/http.h"
#include "speech/speech.h"
#include "log.hpp"

#define VOICE_PATH "voice"
#define SPEECH_ASR "asr.wav"
#define CMD_ETC "command.etc"
#define SIZE 1024
#define SPEECH_TTL "ttl.mp3"
using namespace std;

class turing
{
    private:
        string apiKey="4235c17e20a34e3eb3cad76b5f0d0097";
        string userId="1";
        string url="http://openapi.tuling123.com/openapi/api/v2";
        aip::HttpClient client;
    public:
        turing()
        {

        }
        
        //反序列化
        string ResponsePickup(string& str)
        {
            JSONCPP_STRING errs;
            Json::Value root;
            Json::CharReaderBuilder rb;
            std::unique_ptr<Json::CharReader> const jsonReader(rb.newCharReader());
            bool res=jsonReader->parse(str.data(),str.data()+str.size(),&root,&errs);
            if(!res || !errs.empty())
            {
                LOG(Warning,"jsoncpp parse error");
                return errs;
            }
            Json::Value results = root["results"];
            Json::Value values  = results[0]["values"];
            return values["text"].asString();
        }

        //序列化
        string chat(string message)
        {
            Json::Value root;
            root["reqType"]=0;
            Json::Value word;
            word["text"]=message;
            Json::Value text;
            text["inputText"]=word;
            root["perception"]=text;
            Json::Value user;
            user["apiKey"]=apiKey;
            user["userId"]=userId;
            root["userInfo"]=user;

            Json::StreamWriterBuilder wb;
            std::ostringstream os;

            std::unique_ptr<Json::StreamWriter> jsonWriter(wb.newStreamWriter());
            jsonWriter->write(root,&os);
            string body=os.str();//有了json串
           
            //发送http请求
            string response;
            int code= client.post(url,nullptr,body,nullptr,&response);
            if(code!=CURLcode::CURLE_OK)
            {
                 LOG(Warning,"http 请求错误!");
                 return "";
            }
            return ResponsePickup(response);
        }

        ~turing()
        {

        }
};

class yuzi
{
    private:
        turing tr;
        aip::Speech *client;
        string appid="21527483";
        string apikey="szxkTzGtRhGIGl2MMoE84qIw";
        string secretKey="hZppPdvzecr8WSPSBfh9gPxP6VgB6lqO";
        unordered_map<string,string> record_set;
        string record;
        string play; 
    public:
        yuzi()
        :client(nullptr)
        {

        }
        
        //加载配置文件
        void LoadCommandEtc()
        {
            LOG(Normal,"命令开始执行");
            string name=CMD_ETC;
            ifstream in(name);
            if(!in.is_open())
            {
                LOG(Warning,"Load command etc error");
                exit(1);
            }
            char line[SIZE];
            string sep=": ";
            while(in.getline(line,sizeof(line)))
            {
                string str=line;
                size_t pos=str.find(sep);
                if(pos==string::npos)
                {
                    LOG(Warning,"command etc format error");
                    break;
                }
                string key=str.substr(0,pos);
                string value=str.substr(pos+sep.size());
                key+="。";//语音识别能够成功
                record_set.insert({key,value});
            }
            in.close();
            LOG(Normal,"命令执行成功");
        }
        //初始化
        void Init()
        {
            client=new aip::Speech(appid,apikey,secretKey);
            LoadCommandEtc();
            record="arecord -t wav -c 1 -r 16000 -d 5 -f S16_LE ";
            record+=VOICE_PATH;
            record+="/";
            record+=SPEECH_ASR;
            record+=">/dev/null 2>&1";
            play="cvlc --play-and-exit ";
            play+=VOICE_PATH;
            play+="/";
            play+=SPEECH_TTL;
            play+=">/dev/null 2>&1";
        }
        //执行linux命令
        bool Exec(string command,bool is_print)
        {
            FILE* fp=popen(command.c_str(),"r");
            if(fp==nullptr)
            {
                cout<<"popen erroe"<<endl;
                return false;
            } 
                if(is_print)
                {
                    char c;
                    size_t s=0;
                    while((s=fread(&c,1,1,fp)>0))
                    {
                        cout<<c;
                    }
                } 
            pclose(fp);
            return true;
        }
        
        //反序列化
        string RecognizePickup(Json::Value& root)
        {
            int err_no=root["err_no"].asInt();
            if(err_no!=0)
            {
                cout<<root["err_msg"]<<":"<<err_no<<endl;
                return "unknown";
            }
            return root["result"][0].asString();
        }

        //语音识别
        string ASR(aip::Speech* client)
        {
            string asr_file=VOICE_PATH;
            asr_file+="/";
            asr_file+=SPEECH_ASR;
            
            map<string,string> options;
            string file_content;
            aip::get_file_content(asr_file.c_str(),&file_content);
            Json::Value root=client->recognize(file_content,"wav",16000,options);
            return RecognizePickup(root);
        }
        
        //语音合成
        void TTL(aip::Speech *client,string &str)
        {
            ofstream ofile;
            string ttl=VOICE_PATH;
            ttl+="/";
            ttl+=SPEECH_TTL;
            ofile.open(ttl.c_str(),ios::out | ios::binary);
            string file_ret;
            map<string,string> options;
            options["spd"]="6";
            options["per"]="4";

            Json::Value result =client->text2audio(str,options,file_ret);
            if(!file_ret.empty())
            {
                ofile<<file_ret;
            }
            else
            {
                cout<<result.toStyledString()<<endl;
            }
            ofile.close();
        }

        //判断是否是命令
        bool IsCommand(string& message)
        {
            return record_set.find(message)==record_set.end()? false:true;
        }

        void run()
        {
            while(1)
            {  
                LOG(Normal,"开始录音：");
                fflush(stdout);
                if(Exec(record,false))
                {
                    LOG(Normal,"识别中...");
                    fflush(stdout);
                    string message =ASR(client);
                    cout<<endl;
                    LOG(Normal,message);
                    if(IsCommand(message))
                    {
                        //是命令
                        LOG(Normal,"运行一个指令");
                        Exec(record_set[message],true);
                        continue;
                    }
                    else
                    {
                        //不是命令
                        LOG(Normal,"进行一次正常的对话");
                        cout<<"我："<<message<<endl;
                        string echo=tr.chat(message);
                        cout<<"千反田玉子："<<echo<<endl;
                        //文本转音频(语音合成)
                        TTL(client,echo);
                        Exec(play,false);
                    }
                }
            }   
        }

};
#endif
