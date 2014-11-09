package helpers

import (
    "fmt"
    "errors"
    "io/ioutil"
    "encoding/hex"
    "encoding/binary"
//    "strconv"
    "gopkg.in/yaml.v2"
    "github.com/fuzxxl/freefare/0.3/freefare"
)


func String2aeskey(keydata_str string) (*freefare.DESFireKey, error) {
    keydata := new([16]byte)
    to_keydata, err := hex.DecodeString(keydata_str)
    if err != nil {
        key := freefare.NewDESFireAESKey(*keydata, 0)
        return key, err
    }
    copy(keydata[0:], to_keydata)
    key := freefare.NewDESFireAESKey(*keydata, 0)
    return key,nil
}

func Bytes2aeskey(source []byte) (*freefare.DESFireKey) {
    keydata := new([16]byte)
    copy(keydata[0:], source)
    key := freefare.NewDESFireAESKey(*keydata, 0)
    return key
}

func String2aid(hexdata string) (freefare.DESFireAid, error) {
    nullaid := freefare.NewDESFireAid(uint32(0))
    aidbytes, err := hex.DecodeString(hexdata)
    fmt.Println("String2aid, aidbytes", aidbytes)
    if err != nil {
        return nullaid, err
    }
    aidint, n := binary.Uvarint(aidbytes)
    fmt.Println("String2aid, aidint", aidint)
    if n <= 0 {
        return nullaid, errors.New(fmt.Sprintf("binary.Uvarint returned %d", n))
    }
    aid := freefare.NewDESFireAid(uint32(aidint))
    fmt.Println("String2aid, aid", aid, aid.Aid())
    return aid, nil
}

func Aid2bytes(aid freefare.DESFireAid) []byte {
    ret := make([]byte, 3)
    copy(ret[:], aid[:])
    return ret
}

func String2byte(source string) (byte, error) {
    // TODO: would strconv.ParseUint be less contrived ? probably not...
    bytearray, err := hex.DecodeString(source)
    if err != nil {
        return 0x0, err
    }
    return bytearray[0], nil
}

func Applicationsettings(accesskey byte, frozen, req_auth_fileops, req_auth_dir, allow_master_key_chg bool) byte {
    ret := byte(0)
    ret |= accesskey << 4
    if (frozen) {
        ret |= 1 << 3
    }
    if (req_auth_fileops) {
        ret |= 1 << 2
    }
    if (req_auth_dir) {
        ret |= 1 << 1;
    }
    if (allow_master_key_chg) {
        ret |= 1;
    }
    return ret
}


func LoadYAMLFile(filepath string) (map[interface{}]interface{}, error)  {
    retmap := make(map[interface{}]interface{})

    filedata, err := ioutil.ReadFile(filepath)
    if err != nil {
        return retmap, err
    }

    err = yaml.Unmarshal([]byte(filedata), &retmap)
    if err != nil {
        return retmap, err
    }

    return retmap, nil
}