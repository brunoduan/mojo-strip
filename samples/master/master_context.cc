#include "samples/public/master/master_context.h"

#include "base/unguessable_token.h"

namespace samples {

MasterContext::MasterContext()
    : unique_id_(base::UnguessableToken::Create().ToString()) {
}

MasterContext::~MasterContext() {
}

} // namespace samples
