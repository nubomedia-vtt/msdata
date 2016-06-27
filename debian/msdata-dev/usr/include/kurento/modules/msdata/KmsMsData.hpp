/* Autogenerated with kurento-module-creator */

#ifndef __KMS_MS_DATA_HPP__
#define __KMS_MS_DATA_HPP__

#include <json/json.h>
#include <jsonrpc/JsonRpcException.hpp>
#include <memory>
#include <vector>
#include "MediaElement.hpp"

namespace kurento
{
namespace module
{
namespace msdata
{
class KmsMsData;
} /* msdata */
} /* module */
} /* kurento */

namespace kurento
{
class JsonSerializer;
void Serialize (std::shared_ptr<kurento::module::msdata::KmsMsData> &object, JsonSerializer &serializer);
}

namespace kurento
{
class MediaPipeline;
} /* kurento */

namespace kurento
{
namespace module
{
namespace msdata
{
class KmsMsData : public virtual MediaElement
{

public:
  KmsMsData () {};
  virtual ~KmsMsData () {};

  virtual const std::string& getType () const;
  virtual const std::string& getQualifiedType () const;
  virtual const std::string& getModule () const;
  virtual const std::vector<std::string>& getHierarchy () const;

  virtual void Serialize (JsonSerializer &serializer) = 0;

};

} /* msdata */
} /* module */
} /* kurento */

#endif /*  __KMS_MS_DATA_HPP__ */