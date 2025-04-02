package loxi;

// Using exception for flow control instead of errors: LOVE IT SINS I TRIED IT FOR THE FIRST TIME
class Return extends RuntimeException {
    final Object value;

    Return(Object value) {
        super(null, null, false, false);
        this.value = value;
    }

}
