#pragma once
#include "CSession.h"
#include "Singleton.h"
#include <memory>
#include <mutex>
#include <unordered_map>

class CSession;
class UserMgr : public Singleton<UserMgr> {
  friend class Singleton<UserMgr>;

public:
  ~UserMgr();
  std::shared_ptr<CSession> GetSession(int uid);
  void SetUserSession(int uid, std::shared_ptr<CSession> session);
  void RmvUserSession(int uid);

private:
  UserMgr();
  std::mutex _session_mtx;
  std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};