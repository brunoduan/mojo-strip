package com.xpeng.samples.common;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.echo_master.mojom.EchoMaster;
import org.chromium.echo_master.mojom.EchoMasterConstants;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.Pair;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.services.service_manager.Connector;

/**
 * Implementation of {@link ServiceManagerConnection}
 */
@JNINamespace("samples")
public class ServiceManagerConnectionImpl {
    public interface ConnectionReadyCallback {
        void ready();
    }

    private static ConnectionReadyCallback sCallback;

    private static final String TEST_STRING = "abcdefghijklmnopqrstuvwxyz";
    private static class EchoStringResponseImpl implements EchoMaster.EchoStringResponse {
        @Override
        public void call(String str) {
            assert TEST_STRING.equals(str) == true;
        }
    }

    private static void connectEchoMasterService() {
        MessagePipeHandle handle =
                ServiceManagerConnectionImpl.getConnectorMessagePipeHandle();
        Core core = CoreImpl.getInstance();
        Pair<EchoMaster.Proxy, InterfaceRequest<EchoMaster>> pair =
                EchoMaster.MANAGER.getInterfaceRequest(core);

        // Connect the Echo service via Connector.
        Connector connector = new Connector(handle);
        connector.bindInterface(
                EchoMasterConstants.SERVICE_NAME, EchoMaster.MANAGER.getName(), pair.second);

        // Fire the echoString() mojo call.
        EchoMaster.EchoStringResponse callback = new EchoStringResponseImpl();
        pair.first.echoString(TEST_STRING, callback);
    }

    public static MessagePipeHandle getConnectorMessagePipeHandle() {
        int handle = nativeGetConnectorMessagePipeHandle();
        Core core = CoreImpl.getInstance();
        return core.acquireNativeHandle(handle).toMessagePipeHandle();
    }

    public static void setConnectionReadyCallback(ConnectionReadyCallback callback) {
        sCallback = callback;
    }

    @CalledByNative
    public static void notifyConnectionReady() {
        if (sCallback != null) {
            sCallback.ready();
        }

        connectEchoMasterService();
    }

    private static native int nativeGetConnectorMessagePipeHandle();
}
