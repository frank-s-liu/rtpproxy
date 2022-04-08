#ifndef _SESSION_KEY_H__
#define _SESSION_KEY_H__

class SessionKey
{
public:
    SessionKey(const char* cookie); 
    SessionKey(const char* key, int key_len);
    virtual ~SessionKey();
    bool operator <(const SessionKey& s) const ;

public:
    char*         m_cookie;
    unsigned long m_cookie_id;
    int           m_cookie_len;
};

struct cmp_SessionKey
{
    bool operator()(const SessionKey* l, const SessionKey* r)
    {
        return (*l < *r);
    }
};

#endif
