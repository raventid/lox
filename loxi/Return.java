package loxi;

// Using exception for flow control instead of errors: LOVE IT SINS I FIRST TRIED IT
class Return extends RuntimeException {
    final Object value;

    Return(Object value) {
        super(null, null, false, false);
        this.value = value;
    }

}
