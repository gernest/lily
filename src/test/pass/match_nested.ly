enum class Option[A] {
    Some(A),
    None
}

var opt: Option[Option[integer]] = Some(Some(10))

match opt: {
    case Some(s):
        match s: {
            case Some(s2):
            case None:
                print("Failed.\n")
        }
    case None:
        print("Failed.\n")
}
